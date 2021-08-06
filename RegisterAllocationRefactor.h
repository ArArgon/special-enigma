//
// Created by 廖治平 on 7/31/21.
//

#ifndef SYSYBACKEND_REGISTERALLOCATIONREFACTOR_H
#define SYSYBACKEND_REGISTERALLOCATIONREFACTOR_H

#include "RegisterAllocation.h"

extern bool isDebug;

namespace Backend::RegisterAllocation {
    template<size_t registerCount>
    class ColourAllocatorRewrite : public RegisterAllocator<registerCount> {
        struct VirtualReg {
            IntermediateRepresentation::IROperand operand;
            size_t colour, degree;
            double weight;
            VirtualReg* alias = nullptr;
            std::set<VirtualReg*> adjList;
            std::set<Flow::BasicBlock::BBStatement*> moveList;

            VirtualReg() = default;

            VirtualReg(const IntermediateRepresentation::IROperand &operand) : operand(operand) {
                reset();
            }

            void setPreColour() {
                weight = 0;
                degree = 0x3f3f3f3f;
                alias = nullptr;
                adjList.clear();
                moveList.clear();
            }

            void reset() {
                weight = 0;
                degree = 0;
                colour = 0;
                alias = nullptr;
                adjList.clear();
                moveList.clear();
            }

            void addAdj(VirtualReg* reg) {
                adjList.insert(reg);
                degree++;
            }
        };
        using baseType = RegisterAllocator<registerCount>;
        using bb_t = Flow::bb_ptr_t;

        using is_t = Flow::BasicBlock::BBStatement*;
        using var_t = VirtualReg*;

        std::map<IntermediateRepresentation::IROperand, var_t> toVReg;
        std::vector<bb_t> basicBlocks;
        Flow::ControlFlowGraph cfg;
        std::shared_ptr<Flow::Flow> flowAnalyzer = nullptr;
        std::map<var_t, std::set<is_t>> moveList;
        std::map<var_t, double> weight;
        std::set<is_t> workListMoves, activeMoves, frozenMoves, constrainedMoves, coalescedMoves;
        std::set<var_t> spilledNodes, spillWorklist, colouredNodes, coalescedNodes, freezeWorklist, initial, preColoured, spillTemp;
        std::set<std::pair<var_t, var_t>> adjSet;
        std::list<var_t> simplifyWorklist;
        std::list<var_t> selectStack;

        var_t getVar(const IntermediateRepresentation::IROperand& var) {
            if (!toVReg.count(var)) {
                toVReg[var] = new VirtualReg(var);
                if (isDebug)
                    std::cerr << "New var: " << var.getVarName() << std::endl;
            }
            return toVReg[var];
        }

        void init() {
            initial.clear();
            simplifyWorklist.clear();
            freezeWorklist.clear();
            spillWorklist.clear();
            spilledNodes.clear();
            coalescedNodes.clear();
            colouredNodes.clear();
            selectStack.clear();

            coalescedMoves.clear();
            constrainedMoves.clear();
            frozenMoves.clear();
            activeMoves.clear();

            adjSet.clear();

            if (isDebug) {
                for (auto& bb : basicBlocks) {
                    std::cout << "Statement: " << std::endl;
                    int i = 0;
                    for (auto& ins : bb->statements) {
                        std::cout << ++i << "\t\tlive: ";
                        for (auto& liv : ins.live)
                            std::cout << "[" << liv.toString() <<"], ";
                        std::cout << std::endl;
                    }
                }
            }


            std::set<IntermediateRepresentation::IROperand> nodes;
            for (auto& block : basicBlocks) {
                for (auto& ins : block->statements) {
                    nodes.insert(ins.def.begin(), ins.def.end());
                    nodes.insert(ins.use.begin(), ins.use.end());
                }
            }
            for (auto& node : nodes)
                initial.insert(getVar(node));

            initial = Util::set_diff(initial, preColoured);
            std::for_each(initial.begin(),  initial.end(), [&] (const var_t& var) {
                var->reset();
            });
            std::for_each(preColoured.begin(),  preColoured.end(), [&] (const var_t& var) {
                var->setPreColour();
            });
        }

        void addEdge(const var_t& u, const var_t& v) {
            if (u != v && !adjSet.count({ u, v })) {
                if (isDebug)
                    std::cout << u->operand.getVarName() << " <> " << v->operand.getVarName() << std::endl;
                adjSet.emplace(u, v);
                adjSet.emplace(v, u);
                if (!preColoured.count(u)) {
                    u->addAdj(v);
                }
                if (!preColoured.count(v)) {
                    v->addAdj(u);
                }
            }
        }

        std::set<var_t> adjacent(const var_t& node) {
            std::set<var_t> ans = node->adjList;
            ans = Util::set_diff(ans, coalescedNodes);
            for (auto& selectNode : selectStack)
                if (ans.count(selectNode))
                    ans.erase(selectNode);
            return ans;
        }

        std::set<var_t> adjacent(const var_t& u, const var_t& v) {
            auto ans = u->adjList;
            Util::set_union_to(ans, v->adjList);
            return ans;
        }

        std::set<is_t> nodeMoves(const var_t& node) {
            std::set<is_t> ans;
            auto& list = moveList[node];
            for (auto& move : list) {
                if (activeMoves.count(move) + workListMoves.count(move))
                    ans.insert(move);
            }
            return ans;
        }

        bool moveRelated(const var_t& node) {
            return !nodeMoves(node).empty();
        }

        void enableMoves(const std::set<var_t>& nodes) {
            for (auto node : nodes) {
                for (auto move : nodeMoves(node)) {
                    if (activeMoves.count(move)) {
                        activeMoves.erase(move);
                        workListMoves.insert(move);
                    }
                }
            }
        }

        void decrementDegree(var_t node) {
            if (node->degree-- == registerCount) {
                std::set<var_t> nodes = adjacent(node);
                nodes.insert(node);
                enableMoves(nodes);
                spilledNodes.erase(node);
                if (moveRelated(node))
                    freezeWorklist.insert(node);
                else
                    simplifyWorklist.push_back(node);
            }
        }

        void simplify() {
            var_t node = simplifyWorklist.front();
            simplifyWorklist.pop_front();
            selectStack.push_back(node);
            for (auto adj : adjacent(node))
                decrementDegree(adj);
        }

        void build() {
            auto isMoveIns = [&] (const Flow::BasicBlock::BBStatement* statement) {
                return statement->statement->getStmtType() == IntermediateRepresentation::MOV
                && statement->statement->getOps()[1].getIrOpType() == IntermediateRepresentation::Var;
            };
            for (auto& bb : basicBlocks) {
                auto curLive = bb->liveOut;
                auto& statements = bb->statements;
                for (auto it = statements.rbegin(); it != statements.rend(); it++) {
                    auto ins = is_t { &*it };
                    if (isMoveIns(ins)) {
                        curLive = Util::set_diff(curLive, ins->use);
                        auto moveRegisters = ins->use;
                        moveRegisters.insert(ins->def.begin(), ins->def.end());
                        for (auto& node : moveRegisters)
                            moveList[getVar(node)].insert(ins);
                        workListMoves.insert(ins);
                    }
                    Util::set_union_to(curLive, ins->def);
                    for (auto& def : ins->def)
                        for (auto& l : curLive)
                            addEdge(getVar(l), getVar(def));

                    curLive = Util::set_union(ins->use, Util::set_diff(curLive, ins->def));
                }
            }
        }

        var_t getAlias(var_t n) {
            if (coalescedNodes.count(n)) {
                n->alias = getAlias(n->alias);
                return n->alias;
            } else {
                return n;
            }
        }

        void addWorkList(var_t node) {
            if (!preColoured.count(node) && !moveRelated(node) && node->degree < registerCount) {
                freezeWorklist.erase(node);
                simplifyWorklist.push_back(node);
            }
        }

        bool Ok(var_t t, var_t r) {
            return t->degree < registerCount|| preColoured.count(t) || adjSet.count({ t, r });
        }

        bool forAllOk(var_t u, var_t v) {
            for (var_t w : adjacent(v))
                if (!Ok(w, u))
                    return false;
            return true;
        }

        bool conservative(std::set<var_t> nodes) {
            int cnt = 0;
            for (var_t node : nodes)
                cnt += node->degree >= registerCount;
            return cnt < registerCount;
        }

        void combine(const var_t& u, const var_t& v) {
            if (freezeWorklist.count(v))
                freezeWorklist.erase(v);
            else
                spillWorklist.erase(v);
            coalescedNodes.insert(v);
            if (isDebug)
                std::cout << v->operand.getVarName() << " >> " << u->operand.getVarName() << std::endl;
            v->alias = u;
            Util::set_union_to(u->moveList, v->moveList);
//            std::set<var_t> tmp { v };
//            enableMoves(tmp);
            for (auto t : adjacent(v)) {
                addEdge(t, u);
                decrementDegree(t);
            }
            if (u->degree >= registerCount && freezeWorklist.count(u)) {
                freezeWorklist.erase(u);
                spillWorklist.insert(u);
            }
        }

        void coalesce() {
            auto move = *workListMoves.begin();

            var_t x = getAlias(getVar(move->statement->getOps()[0])),
                  y = getAlias(getVar(move->statement->getOps()[1])),
                  u, v;
            if (preColoured.count(y))
                u = y, v = x;
            else
                u = x, v = y;
            workListMoves.erase(move);
            if (u == v) {
                coalescedMoves.insert(move);
                addWorkList(u);
            } else if (preColoured.count(v) || adjSet.count({ u, v })) {
                constrainedMoves.insert(move);
                addWorkList(u);
                addWorkList(v);
            } else {
                if ((preColoured.count(u) && forAllOk(u, v)) || (!preColoured.count(u) && conservative(adjacent(u, v)))) {
                    coalescedMoves.insert(move);
                    combine(u, v);
                    addWorkList(u);
                } else {
                    activeMoves.insert(move);
                }
            }
        }

        void freeze() {
            var_t u = *freezeWorklist.begin();
            freezeWorklist.erase(u);
            simplifyWorklist.push_back(u);
            freezeMoves(u);
        }

        void freezeMoves(var_t u) {
            for (auto move : nodeMoves(u)) {
                auto& ops = move->statement->getOps();
                var_t x = getVar(ops[0]), y = getVar(ops[1]), v;
                if (getAlias(u) == getAlias(y))
                    v = getAlias(x);
                else
                    v = getAlias(y);
                if (activeMoves.count(move))
                    activeMoves.erase(move);
                else
                    workListMoves.erase(move);
                frozenMoves.insert(move);
//                if (v->degree < registerCount && nodeMoves(v).empty()) {
                if (v->degree < registerCount && !moveRelated(v)) {
                    freezeWorklist.erase(v);
                    simplifyWorklist.push_back(v);
                }
            }
        }

        void selectSpill() {
            var_t toSpill = nullptr;
            double minCost = 1e9;
            bool hasCandidate = false;
            for (auto it = spillWorklist.begin(); it != spillWorklist.end(); it++) {
                // cost = weight / degree
                double cost = weight[*it] / (*it)->degree;
                if (!spillTemp.count(*it)) {
                    hasCandidate = true;
                    if (cost < minCost) {
                        minCost = cost;
                        toSpill = *it;
                    }
                } else if (!hasCandidate) {
                    toSpill = *it;
                }
            }
            spillWorklist.erase(toSpill);
            simplifyWorklist.push_back(toSpill);
            freezeMoves(toSpill);
        }

        void assignColours() {
            while (!selectStack.empty()) {
                var_t node = selectStack.back();
                selectStack.pop_back();
                std::set<size_t> okColours;
                for (size_t i = 0; i < registerCount; i++)
                    okColours.insert(i);

                std::set<var_t> coloured = colouredNodes;
                Util::set_union_to(coloured, preColoured);
                for (var_t w : node->adjList) {
                    if (coloured.count(getAlias(w)))
                        okColours.erase(getAlias(w)->colour);
                }
                if (okColours.empty()) {
                    spilledNodes.insert(node);
                    if (isDebug)
                        std::cout << "Spilling: " << node->operand.getVarName() << std::endl;
                }
                else {
                    colouredNodes.insert(node);
                    node->colour = *okColours.begin();
                }
            }
            for (var_t node : coalescedNodes)
                node->colour = getAlias(node)->colour;
        }

        void doFunctionScan() override {
            while (true) {
                init();

                flowAnalyzer = std::make_shared<Flow::Flow>(Flow::Flow { baseType::sourceFunc });
                cfg = flowAnalyzer->getCfg();
                basicBlocks = flowAnalyzer->getBasicBlocks();

                build();
                for (var_t node : initial) {
                    if (node->degree >= registerCount)
                        spillWorklist.insert(node);
                    else if (moveRelated(node))
                        freezeWorklist.insert(node);
                    else
                        simplifyWorklist.push_back(node);
                }
                do {
                    if (!simplifyWorklist.empty())
                        simplify();
                    else if (!workListMoves.empty())
                        coalesce();
                    else if (!freezeWorklist.empty())
                        freeze();
                    else if (!spillWorklist.empty())
                        selectSpill();
                } while (!(
                        simplifyWorklist.empty() && workListMoves.empty() &&
                        freezeWorklist.empty() && spillWorklist.empty()
                ));
                assignColours();
                if (spilledNodes.empty())
                    break;
                rewriteFunction();
            }
        }

        void rewriteFunction() {
            if (isDebug)
                std::cerr << "Rewrite function not implemented" << std::endl;
            auto isAllocated = [&] (const IntermediateRepresentation::IROperand& var) {
                return baseType::stackScheme->isInStack(getAlias(getVar(var))->operand);
            };

            // assign stack space
            for (auto& node : spilledNodes) {
                baseType::stackScheme->allocate(node->operand);
            }

            for (auto& bb : basicBlocks) {
                for (auto& ins : bb->statements) {
                    if (ins.def.size() == 1) {
                        getAlias(getVar(*ins.def.begin()));
                    }
                }
            }

            for (auto& bb : basicBlocks) {
                auto& ins = bb->statements;
                for (auto it = ins.begin(); it != ins.end(); it++) {
                    if (!it->init)
                        continue;
                    auto& ops = it->statement->getRefOps();
                    auto stmtType = it->statement->getStmtType();
                    auto afterIt = it;
                    for (auto& useVar : it->use) {
                        if (isAllocated(useVar)) {
                            size_t offset = baseType::stackScheme->getVariablePosition(useVar);
                            if (it->def.count(useVar)) {
                                IntermediateRepresentation::IROperand tmpVar {
                                    IntermediateRepresentation::i32, "tmp_" + std::to_string(baseType::spilledCount++) + "_" + useVar.getVarName()
                                };
                                // insert before
                                // stk_load     %xxx#<var>, %off
                                Flow::BasicBlock::BBStatement tmpStmt;
                                auto load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_LOAD, IntermediateRepresentation::i32, useVar, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, offset));
                                tmpStmt.statement = new IntermediateRepresentation::Statement(load_st);
                                it = ins.insert(it, tmpStmt) + 1;

                                // insert after
                                // stk_str      %xxx#<var>, %off
                                load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_STR, IntermediateRepresentation::i32, useVar, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, offset));
                                tmpStmt.statement = new IntermediateRepresentation::Statement(load_st);
                                afterIt = ins.insert(afterIt + 1, tmpStmt);

                                // replace usage
                                it->replaceUse(useVar, tmpVar);
                                it->replaceDef(useVar, tmpVar);
                                spillTemp.insert(getVar(tmpVar));
                            } else {
                                if (stmtType == IntermediateRepresentation::MOV && ops[0] == useVar && isAllocated(ops[1])) {
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
                                        IntermediateRepresentation::i32, "tmp_" + std::to_string(baseType::spilledCount++) + "_" + useVar.getVarName()
                                    };

                                    // insert before
                                    // stk_load     %tmp_xxx_<var>, %off
                                    Flow::BasicBlock::BBStatement tmpStmt;
                                    auto load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_LOAD, IntermediateRepresentation::i32, useVar, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, offset));
                                    tmpStmt.statement = new IntermediateRepresentation::Statement(load_st);
                                    it = ins.insert(it, tmpStmt) + 1;

                                    it->replaceUse(useVar, tmpVar);
                                }
                            }
                        }
                    }
                    for (auto& defVar : it->def) {
                        if (isAllocated(defVar)) {
                            auto offset = baseType::stackScheme->getVariablePosition(defVar);
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
                                    IntermediateRepresentation::i32, "tmp_" + std::to_string(baseType::spilledCount++) + "_" + defVar.getVarName()
                                };

                                // insert after
                                /*
                                 * stk_str      %dest,  %off
                                 * */
                                Flow::BasicBlock::BBStatement tmpStmt;
                                auto load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_STR, IntermediateRepresentation::i32, defVar, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, offset));
                                tmpStmt.statement = new IntermediateRepresentation::Statement(load_st);
                                afterIt = ins.insert(afterIt + 1, tmpStmt);

                                spillTemp.insert(getVar(defVar));
                                it->replaceDef(defVar, tmpVar);
                            }
                        }
                    }
                    it = afterIt;
                }
            }

            spilledNodes.clear();
            initial = colouredNodes;

            colouredNodes.clear();
            coalescedNodes.clear();
        }

        void loadPrecolour() {
            auto& stmts = baseType::sourceFunc->getStatements();
            auto& params = baseType::sourceFunc->getParameters();

            // load pre-colour
            //
            /*
             * call %dest, func, %1, %2, %3, %4, %5, ..., %N
             *
             * %dest is pre-coloured: r0
             * %1, %2, %3, %4 = r0, r1, r2, r3
             * */

            // function parameters
            for (int i = 0; i < std::min(4, (int) params.size()); i++) {
                var_t var = getVar(params[i]);
                preColoured.insert(var);
                var->colour = i;
            }

            for (auto& stmt : stmts) {
                auto& ops = stmt.getOps();
                const int opsCount = static_cast<int>(ops.size());
                if (stmt.getStmtType() == IntermediateRepresentation::CALL) {
                    // pre-colour %dest
                    var_t var;
                    if (ops[0].getIrOpType() == IntermediateRepresentation::Var) {
                        var = getVar(ops[0]);
                        preColoured.insert(var);
                        var->colour = 0;
                    }
                    for (int i = 2; i < std::min(6, opsCount); i++) {
                        var = getVar(ops[i]);
                        if (ops[i].getIrOpType() == IntermediateRepresentation::Var) {
                            preColoured.insert(var);
                            // op1 -> r0, op2 -> r1, ...
                            var->colour = i - 2;
                        }
                    }
                } else if (stmt.getStmtType() == IntermediateRepresentation::RETURN) {
                    if (stmt.getDataType() != IntermediateRepresentation::t_void && ops[0].getIrOpType() == IntermediateRepresentation::Var) {
                        var_t var = getVar(ops[0]);
                        // return   %xxx
                        preColoured.insert(var);
                        var->colour = 0;
                    }
                }
            }
        }

        void saveFunction() {
            std::vector<IntermediateRepresentation::Statement> stmts;
            for (auto &bb : basicBlocks) {
                for (auto &ins : bb->statements) {
                    stmts.push_back(*ins.statement);
                }
            }
            baseType::sourceFunc->setStatements(stmts);
        }

    public:

        ColourAllocatorRewrite(Util::StackScheme* stack, IntermediateRepresentation::Function* func)  :
        baseType::RegisterAllocator(stack, func) {
            loadPrecolour();

            // Flow & Liveness analysis
            flowAnalyzer = std::make_shared<Flow::Flow>(Flow::Flow { baseType::sourceFunc });
            cfg = flowAnalyzer->getCfg();
            basicBlocks = flowAnalyzer->getBasicBlocks();

//            std::cout << "Pre-colour scheme: " << std::endl;
//            for (auto& node : preColourScheme)
//                std::cout << "\t" << node.first.toString() << ": " << node.second << std::endl;

            doFunctionScan();
            saveFunction();
            if (isDebug)
                std::cerr << "toVReg Count: " << toVReg.size() << std::endl << "";
            for (auto var : toVReg) {
                baseType::variables.insert(var.first);
                baseType::allocation[var.first] = var.second->colour;
            }
        }

        ~ColourAllocatorRewrite() = default;
    };
}

#endif //SYSYBACKEND_REGISTERALLOCATIONREFACTOR_H
