//
// Created by 廖治平 on 7/18/21.
//

#include "RegisterAllocation.h"


namespace Backend::RegisterAllocation {
    /*
     * Logic check: Pass
     * */
    template<size_t registerCount>
    void ColourAllocator<registerCount>::coalesce() {
        /*
         * Logic check: Pass
         * */
        auto conservative = [&] (const var_t& u, const var_t& v) {
            int cnt = 0;
            auto nodes = Util::set_union(interferenceGraph.getNeighbours(u), interferenceGraph.getNeighbours(v));
            for (auto& n : nodes) {
                cnt += interferenceGraph.getNodeDegree(n) >= registerCount;
            }
            return cnt < registerCount;
        };

        /*
         * Logic check: Pass
         * */
        auto isOk = [&] (const var_t& t, const var_t& r) {
            return interferenceGraph.getNodeDegree(t) < registerCount || preColoured.count(t) || interferenceGraph.containsEdge(t, r);
        };

        /*
         * Logic check: Pass
         * */
        auto addWorkList = [&] (const var_t& u) {
            if (!preColoured.count(u) && nodeMoves(u).empty() && interferenceGraph.getNodeDegree(u) < registerCount) {
                freezeWorklist.erase(u);
                simplifyWorklist.push_back(u);
            }
        };

        auto& m = *workListMoves.begin();
        workListMoves.erase(workListMoves.begin());
        auto x = alias.find(m->statement->getOps()[0]), y = alias.find(m->statement->getOps()[1]);
        if (preColoured.count(y))
            std::swap(x, y);
        if (x == y) {
            // mov x, y ?
            coalescedMoves.insert(m);
            addWorkList(x);
        } else if (preColoured.count(y) || interferenceGraph.containsEdge(x, y)) {
            constrainedMoves.insert(m);
            addWorkList(x);
            addWorkList(y);
        } else {
            bool ok = true;

            for (auto& suc : interferenceGraph.getNeighbours(y))
                ok &= isOk(suc, x);

            if ((ok && preColoured.count(x)) || (!preColoured.count(x) && conservative(x, y))) {
                coalescedMoves.insert(m);
                combine(x, y);
                addWorkList(x);
            } else
                activeMoves.insert(m);
        }
    }

    /*
     * Logic check: Pass
     * */
    template<size_t registerCount>
    void ColourAllocator<registerCount>::buildIntGraph() {
        auto isMoveRelated = [] (const std::shared_ptr<Backend::Flow::BasicBlock::BBStatement>& statement) {
            return statement->statement->getStmtType() == IntermediateRepresentation::MOV;
        };
//        moveList.clear();
//        workListMoves.clear();

        for (auto& bb : basicBlocks) {
            auto live = bb->liveOut;
            auto& statements = bb->statements;
            for (int i = static_cast<int>(statements.size()) - 1; i >= 0; i--) {
                auto&& ins = is_t { &statements[i] };
                if (isMoveRelated(ins)) {
                    live = Util::set_diff(live, ins->use);
                    auto moveRelated = ins->use;
                    moveRelated.insert(ins->def.begin(), ins->def.end());
                    for (auto& node : moveRelated)
                        moveList[node].insert(ins);
                    workListMoves.insert(ins);
                }
                Util::set_union_to(live, ins->def);
                for (auto& def : ins->def)
                    for (auto& liv : live)
                        interferenceGraph.addBiEdge(def, liv);

                live = Util::set_union(ins->use, Util::set_diff(live, ins->def));
            }
        }
    }

    template<size_t registerCount>
    void ColourAllocator<registerCount>::rewriteFunction() {
        /*
         * TODO: For each spilled nodes, create new temporary vi for each definition and each use
         * */

        auto isAllocated = [&] (const IntermediateRepresentation::IROperand& var) {
            return baseType::stackScheme->isInStack(alias.find(var));
        };

        // assign stack space
        for (auto& node : spilledNodes) {
            baseType::stackScheme->allocate(node);
        }

        for (auto& bb : basicBlocks) {
            auto& ins = bb->statements;
            for (auto it = ins.begin(); it != ins.end(); it++) {
                auto& ops = it->statement->getRefOps();
                auto stmtType = it->statement->getStmtType();
                for (auto& useVar : it->use) {
                    if (isAllocated(useVar)) {
                        size_t offset = baseType::stackScheme->getVariablePosition(useVar);
                        if (it->def.count(useVar)) {
                            IntermediateRepresentation::IROperand tmpVar {
                                IntermediateRepresentation::i32, "tmp_" + baseType::spilledCount++ + "_" + useVar.getVarName()
                            };
                            // insert before
                            // stk_load     %xxx#<var>, %off
                            Flow::BasicBlock::BBStatement tmpStmt;
                            auto load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_LOAD, IntermediateRepresentation::i32, useVar, offset);
                            tmpStmt.statement = std::make_shared<decltype(load_st)>(load_st);
                            it = ins.insert(it, tmpStmt) + 1;

                            // insert after
                            // stk_str      %xxx#<var>, %off
                            load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_STR, IntermediateRepresentation::i32, useVar, offset);
                            tmpStmt.statement = std::make_shared<decltype(load_st)>(load_st);
                            it = ins.insert(it + 1, tmpStmt) + 1;

                            // replace usage
                            it->replaceUse(useVar, tmpVar);
                            it->replaceDef(useVar, tmpVar);
                            spillTemp.insert(tmpVar);
                        } else {
                            if (stmtType == IntermediateRepresentation::MOV && ops[0] == useVar && !isAllocated(ops[1])) {
                                /*
                                 * mov          %dest, %src     @ %src is allocated, and %dest is not.
                                 *
                                 * stk_load     %dest, %<src.off>
                                 * */
                                it->statement->setStmtType(IntermediateRepresentation::STK_LOAD);
                                it->statement->setDataType(IntermediateRepresentation::i32);
                                ops[2] = IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, offset);
                            } else {
                                /*
                                 * opr          %arg1, %argX, ..., %argN
                                 *
                                 * stk_load     %tmp_<id>_argX, %off
                                 * opr          %arg1, %tmp_<id>_argX, ...
                                 * */
                                IntermediateRepresentation::IROperand tmpVar {
                                        IntermediateRepresentation::i32, "tmp_" + baseType::spilledCount++ + "_" + useVar.getVarName()
                                };

                                // insert before
                                // stk_load     %tmp_xxx_<var>, %off
                                Flow::BasicBlock::BBStatement tmpStmt;
                                auto load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_LOAD, IntermediateRepresentation::i32, useVar, offset);
                                tmpStmt.statement = std::make_shared<decltype(load_st)>(load_st);

                                it = ins.insert(it, tmpStmt) + 1;
                                it->replaceUse(useVar, tmpVar);
                            }
                        }
                    }
                }
                for (auto& defVar : it->def) {
                    if (isAllocated(defVar)) {
                        auto offset = baseType::stackScheme.getVariablePosition(defVar);
                        if (stmtType == IntermediateRepresentation::MOV && !isAllocated(ops[1])) {
                            /*
                             * mov      %dest, %src     @ %dest is allocated, and %src is not allocated or an imm.
                             *
                             * stk_str  %src, %off
                             * */
                            it->statement->setDataType(IntermediateRepresentation::i32);
                            it->statement->setStmtType(IntermediateRepresentation::STK_STR);
                            it->statement->setOps({ ops[1], IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, offset) });
                        } else {
                            IntermediateRepresentation::IROperand tmpVar {
                                    IntermediateRepresentation::i32, "tmp_" + baseType::spilledCount++ + "_" + defVar.getVarName()
                            };

                            // insert after
                            /*
                             * stk_str      %dest,  %off
                             * */
                            Flow::BasicBlock::BBStatement tmpStmt;
                            auto load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_STR, IntermediateRepresentation::i32, defVar, offset);
                            tmpStmt.statement = std::make_shared<decltype(load_st)>(load_st);
                            it = ins.insert(it + 1, tmpStmt) + 1;

                            spillTemp.insert(defVar);
                            it->replaceDef(defVar, tmpVar);
                        }
                    }
                }
            }
        }

        spilledNodes.clear();
        initial = coloredNodes;

        coloredNodes.clear();
        coalescedNodes.clear();
    }

    /*
     * Logic check: Pass
     * */
    template<size_t registerCount>
    void ColourAllocator<registerCount>::simplify() {
        auto node = simplifyWorklist.front();
        simplifyWorklist.pop_front();
        selectStack.push(node);
        auto&& neighbours = interferenceGraph.getNeighbours(node);
        for (auto& suc : neighbours) {
            decrementDegree(suc);
        }
    }

    /*
     * Logic check: Pass
     * */
    template<size_t registerCount>
    void ColourAllocator<registerCount>::decrementDegree(const ColourAllocator::var_t& node) {
        size_t degree = interferenceGraph.getNodeDegree(node);
        interferenceGraph.setNodeDegree(node, degree - 1);
        if (degree == registerCount) {
            // EnableMoves
            enableMoves(node);
            spillWorklist.erase(node);
            if (!moveList[node].empty())
                freezeWorklist.insert(node);
            else
                simplifyWorklist.push_back(node);
        }
    }

    /*
     * Logic check: Pass
     * */
    template<size_t registerCount>
    void ColourAllocator<registerCount>::enableMoves(const ColourAllocator::var_t& node) {
        std::set<var_t> nodes = interferenceGraph.getNeighbours(node);
        nodes.insert(node);

        for (auto& n : nodes) {
            auto&& moves = nodeMoves(n);
            for (auto& m : moves) {
                if (activeMoves.count(m)) {
                    activeMoves.erase(m);
                    workListMoves.insert(m);
                }
            }
        }
    }

    /*
     * Logic check: Pass
     * */
    template<size_t registerCount>
    std::set<std::shared_ptr<Flow::BasicBlock::BBStatement>> ColourAllocator<registerCount>::nodeMoves(const ColourAllocator::var_t & node) {
        std::set<is_t> ans;
        auto& list = moveList[node];
        for (auto& move : list) {
            if (activeMoves.count(move) + workListMoves.count(move))
                ans.insert(move);
        }
        return ans;
    }

    /*
     * Logic check: Stand-by, "initial"
     * */
    template<size_t registerCount>
    void ColourAllocator<registerCount>::buildWorkLists() {
        for (auto& n : initial) {
            if (interferenceGraph.getNodeDegree(n) >= registerCount) {
                spillWorklist.insert(n);
            } else if (moveList.count(n)) {
                freezeWorklist.insert(n);
            } else {
                simplifyWorklist.push_back(n);
            }
        }
    }

    /*
     * Logic check: Pass
     *
     * Remark: colour mechanism could be changed.
     * */
    template<size_t registerCount>
    void ColourAllocator<registerCount>::assignColours() {
        for (auto& pre : preColoured)
            colour[pre] = preColourScheme[pre];
        while (!selectStack.empty()) {
            auto& n = selectStack.top();
            std::set<size_t> okColours;
            for (size_t i = 0; i < registerCount; i++)
                okColours.insert(i);

            selectStack.pop();
            auto&& neighbours = interferenceGraph.getNeighbours(n);
            for (auto& w : neighbours) {
                auto&& fa = alias.find(w);
                if (colour.count(fa))
                    okColours.erase(colour[fa]);
            }
            // No colours left
            if (okColours.empty())
                spilledNodes.insert(n);
            else {
                // colour this node
                coloredNodes.insert(n);
                colour[n] = *okColours.begin();
            }
        }
    }

    /*
     * Logic check: Pass
     * */
    template<size_t registerCount>
    void ColourAllocator<registerCount>::freeze() {
        auto& u = *freezeWorklist.begin();
        freezeWorklist.erase(freezeWorklist.begin());
        simplifyWorklist.push_back(u);
        freezeMoves(u);
    }

    /*
     * Logic check: Pass
     * */
    template<size_t registerCount>
    void ColourAllocator<registerCount>::freezeMoves(const var_t& u) {
        auto&& moves = nodeMoves(u);
        for (auto& m : moves) {
            if (activeMoves.count(m))
                activeMoves.erase(m);
            else
                workListMoves.erase(m);
            frozenMoves.insert(m);

            auto&& ops = m->statement->getOps();
            auto& x = ops[0], y = ops[1];
            if (x != u)
                std::swap(x, y);
            if (nodeMoves(y).empty() && interferenceGraph.getNodeDegree(y) < registerCount) {
                freezeWorklist.erase(y);
                simplifyWorklist.push_back(y);
            }
        }
    }

    /**
     * Logic check: Stand-by
     * Choose a move to spill
     * */
    template<size_t registerCount>
    void ColourAllocator<registerCount>::selectSpill() {
        /*
         * Select an ideal move from spillWorkList
         * */
        double minCost = 1e10;
        bool hasCandidate = false;
        auto opt = spillWorklist.begin();
        for (auto it = spillWorklist.begin(); it != spillWorklist.end(); it++) {
            // cost = weight / degree
            double cost = weight[*it] / interferenceGraph.getNodeDegree(*it);
            // TODO imm
            if (!spillTemp.count(*it)) {
                hasCandidate = true;
                if (cost < minCost) {
                    minCost = cost;
                    opt = it;
                }
            } else if (!hasCandidate) {
                opt = it;
            }
        }

        auto& m = *opt;
        spillWorklist.erase(opt);
        simplifyWorklist.push_back(m);
        freezeMoves(m);
    }

    /*
     * Logic check: Pass
     * */
    template<size_t registerCount>
    void ColourAllocator<registerCount>::combine(const ColourAllocator::var_t &u, const ColourAllocator::var_t &v) {
        if (freezeWorklist.count(v))
            freezeWorklist.erase(v);
        else
            spillWorklist.erase(v);
        coalescedNodes.insert(v);
        int ret = alias.disjoint(v, u);
        /*
         * Could rewrite if depth is enabled
         * */
        Util::set_union_to(moveList[u], moveList[v]);
        auto&& adj = interferenceGraph.getNeighbours(v);
        for (auto& t : adj) {
            interferenceGraph.addBiEdge(t, u);
            decrementDegree(t);
        }
        if (interferenceGraph.getNodeDegree(u) >= registerCount && freezeWorklist.count(u)) {
            freezeWorklist.erase(u);
            spillWorklist.insert(u);
        }
    }

}
