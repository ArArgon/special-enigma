//
// Created by 廖治平 on 7/18/21.
//

#ifndef SYSYBACKEND_IRREGISTERALLOCATION_H
#define SYSYBACKEND_IRREGISTERALLOCATION_H

#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <memory>
#include <list>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include "IRTypes.h"
#include "Utilities.h"
#include "Flow.h"
#include "InstructionOperands.h"

namespace Backend::RegisterAllocation {
    // A virtual class for register allocation algorithms

    template<size_t registerCount>
    class RegisterAllocator {
    protected:
        std::shared_ptr<IntermediateRepresentation::Function> sourceFunc;
        std::map<IntermediateRepresentation::IROperand, size_t> allocation;
        std::set<IntermediateRepresentation::IROperand> variables;
        std::shared_ptr<Util::StackScheme> stackScheme;
        std::unordered_set<size_t> totalColours; // TODO

        size_t spilledCount = 0;

        virtual void doFunctionScan() = 0;
    public:

        explicit RegisterAllocator(Util::StackScheme &stack, std::shared_ptr<IntermediateRepresentation::Function> func) : stackScheme(&stack), sourceFunc(std::move(func)) { };
        RegisterAllocator() = default;
        virtual ~RegisterAllocator() = default;

        const auto& getAllocation() { return allocation; }

        const auto& getVariables() { return variables; }

        const std::shared_ptr<Util::StackScheme> &getStackScheme() const { return stackScheme; }

        const std::unordered_set<size_t> &getTotalColours() const { return totalColours; }
    };

    template<size_t registerCount>
    class LinearScan : public RegisterAllocator<registerCount> {

    };

    template<size_t registerCount>
    class ColourAllocator : public RegisterAllocator<registerCount> {

        using baseType = RegisterAllocator<registerCount>;
        using bb_t = Flow::bb_ptr_t;

        using is_t = Flow::BasicBlock::BBStatement*;
        using var_t = IntermediateRepresentation::IROperand;
        using InterferenceGraph = Util::Graph<var_t>;

//        std::shared_ptr<IntermediateRepresentation::Function> func;
        std::vector<bb_t> basicBlocks;
        Flow::ControlFlowGraph cfg;
        InterferenceGraph interferenceGraph;
        std::shared_ptr<Flow::Flow> flowAnalyzer = nullptr;
        std::unordered_map<var_t, std::unordered_set<is_t>> moveList;
        std::unordered_map<var_t, size_t> colour; // TODO
        std::unordered_map<var_t, double> weight;
        std::unordered_set<is_t> workListMoves, activeMoves, frozenMoves, constrainedMoves, coalescedMoves;
        std::unordered_set<var_t> spilledNodes, spillWorklist, coloredNodes, coalescedNodes, freezeWorklist, initial, preColoured, spillTemp;
        std::unordered_map<var_t, size_t> preColourScheme;
        std::list<var_t> simplifyWorklist;
        std::list<var_t> selectStack;
        Util::DisjointSet<var_t> alias;

        /*
         * Logic check: Pass
         * */
        std::set<is_t> nodeMoves(const var_t& node) {
            std::set<is_t> ans;
            auto& list = moveList[node];
            for (auto& move : list) {
                if (activeMoves.count(move) + workListMoves.count(move))
                    ans.insert(move);
            }
            return ans;
        }

        std::unordered_set<var_t> adjacent(const var_t& node) {
            std::unordered_set<var_t> ans = interferenceGraph.getNeighbours(node);
            ans = Util::set_diff(ans, coalescedNodes);
            for (auto& selectNode : selectStack)
                if (ans.count(selectNode))
                    ans.erase(selectNode);
            return ans;
        }

        /*
         * Logic check: Pass
         * */
        void enableMoves(const var_t& node) {
            std::unordered_set<var_t> nodes = adjacent(node);
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
        void decrementDegree(const var_t& node) {
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
        void combine(const var_t& u, const var_t& v) {
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
            auto&& adj = adjacent(v);
            for (auto& t : adj) {
                interferenceGraph.addBiEdge(t, u);
                decrementDegree(t);
            }
            if (interferenceGraph.getNodeDegree(u) >= registerCount && freezeWorklist.count(u)) {
                freezeWorklist.erase(u);
                spillWorklist.insert(u);
            }
        }

        /*
         * Logic check: Pass
         * */
        void coalesce() {
            /*
             * Logic check: Pass
             * */
            auto conservative = [&] (const var_t& u, const var_t& v) {
                int cnt = 0;
                auto nodes = Util::set_union(adjacent(u), adjacent(v));
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

            auto m = *workListMoves.begin();
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

                for (auto& suc : adjacent(y))
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
        void freeze() {
            auto u = *freezeWorklist.begin();
            freezeWorklist.erase(freezeWorklist.begin());
            simplifyWorklist.push_back(u);
            freezeMoves(u);
        }

        /*
         * Logic check: Pass
         * */
        void freezeMoves(const var_t u) {
            auto&& moves = nodeMoves(u);
            for (auto& m : moves) {
                auto& ops = m->statement->getOps();
                auto x = ops[0], y = ops[1];
                if (alias.find(u) == alias.find(y))
                    y = alias.find(x);
                else
                    y = alias.find(y);

                if (activeMoves.count(m))
                    activeMoves.erase(m);
                else
                    workListMoves.erase(m);
                frozenMoves.insert(m);
                if (nodeMoves(y).empty() && interferenceGraph.getNodeDegree(y) < registerCount) {
                    freezeWorklist.erase(y);
                    simplifyWorklist.push_back(y);
                }
            }
        }

        /*
         * Logic check: Pass
         * */
        void simplify() {
            auto node = simplifyWorklist.front();
            simplifyWorklist.pop_front();
            selectStack.push_back(node);
            auto&& adj = adjacent(node);
            for (auto& suc : adj) {
                decrementDegree(suc);
            }
        }

        /*
         * Logic check: Pass
         *
         * Remark: colour mechanism could be changed.
         * */
        void assignColours() {
            for (auto& pre : preColoured)
                colour[pre] = preColourScheme[pre];
            while (!selectStack.empty()) {
                auto n = *selectStack.rbegin();
                selectStack.pop_back();
                std::set<size_t> okColours;
                for (size_t i = 0; i < registerCount; i++)
                    okColours.insert(i);

                auto&& neighbours = interferenceGraph.getNeighbours(n);
                for (auto& w : neighbours) {
                    auto&& fa = alias.find(w);
                    if (colour.count(fa) || preColoured.count(fa))
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

            for (auto& node : coalescedNodes) {
                auto fa = alias.find(node);
                if (fa != node)
                    colour[node] = colour[fa];
            }
        }

        /*
         * Logic check: Stand-by, "initial"
         * */
        void buildWorkLists() {
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

        /**
         * Logic check: Stand-by
         * Choose a move to spill
         * */
        void selectSpill() {
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

            auto m = *opt;
            spillWorklist.erase(opt);
            simplifyWorklist.push_back(m);
            freezeMoves(m);
        }

        /*
         * Logic check: Pass
         * */
        void buildIntGraph() {
            auto isMoveRelated = [&] (const Flow::BasicBlock::BBStatement* statement) {
                return statement->statement->getStmtType() == IntermediateRepresentation::MOV
                        && statement->statement->getOps()[1].getIrOpType() == IntermediateRepresentation::Var;
            };
//        moveList.clear();
//        workListMoves.clear();

            for (auto& bb : basicBlocks) {
                auto live = bb->liveOut;
                auto& statements = bb->statements;
                for (int i = static_cast<int>(statements.size()) - 1; i >= 0; i--) {
                    auto ins = is_t { &statements[i] };
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

        void rewriteFunction() {
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
                                        IntermediateRepresentation::i32, "tmp_" + std::to_string(baseType::spilledCount++) + "_" + useVar.getVarName()
                                };
                                // insert before
                                // stk_load     %xxx#<var>, %off
                                Flow::BasicBlock::BBStatement tmpStmt;
                                auto load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_LOAD, IntermediateRepresentation::i32, useVar, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, offset));
                                tmpStmt.statement = std::make_shared<decltype(load_st)>(load_st);
                                it = ins.insert(it, tmpStmt) + 1;

                                // insert after
                                // stk_str      %xxx#<var>, %off
                                load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_STR, IntermediateRepresentation::i32, useVar, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, offset));
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
                                            IntermediateRepresentation::i32, "tmp_" + std::to_string(baseType::spilledCount++) + "_" + useVar.getVarName()
                                    };

                                    // insert before
                                    // stk_load     %tmp_xxx_<var>, %off
                                    Flow::BasicBlock::BBStatement tmpStmt;
                                    auto load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_LOAD, IntermediateRepresentation::i32, useVar, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, offset));
                                    tmpStmt.statement = std::make_shared<decltype(load_st)>(load_st);

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

        void init() {
            // flow analysis
            flowAnalyzer = std::make_shared<Flow::Flow>(Flow::Flow { baseType::sourceFunc });
            cfg = flowAnalyzer->getCfg();
            basicBlocks = flowAnalyzer->getBasicBlocks();

            // structures
            interferenceGraph = InterferenceGraph();
            moveList.clear();
            workListMoves.clear();
            spilledNodes.clear();
            freezeWorklist.clear();
            spillWorklist.clear();
            simplifyWorklist.clear();
            coalescedNodes.clear();
            coloredNodes.clear();
            initial.clear();
            while(!selectStack.empty())
                selectStack.pop_back();
            coalescedMoves.clear();
            constrainedMoves.clear();
            frozenMoves.clear();
            activeMoves.clear();
            // build initial & alias

            for (auto& block : basicBlocks) {
                for (auto& ins : block->statements) {
                    initial.insert(ins.def.begin(), ins.def.end());
                    initial.insert(ins.use.begin(), ins.use.end());
                }
            }

            alias = Util::DisjointSet<var_t>(initial);
            initial = Util::set_diff(initial, preColoured);
//            alias = Util::DisjointSet<var_t> { interferenceGraph.getNodes() };

            // calculate weight
            for (auto& block : basicBlocks) {
                // 10 ^ min(count_of(precursors), count_of(succesors))
                double curWeight = std::pow(10.0, std::min(cfg.getPrecursorsOf(block).size(), cfg.getSuccessorsOf(block).size()));
                auto&& statements = block->getStatements();
                for (auto& ins : statements) {
                    auto&& variables = Util::set_union(ins.use, ins.def);
                    for (auto& var : variables)
                        weight[var] += curWeight;
                }
            }
        }

        /*
         * Logic check: pass
         * */
        void doFunctionScan() override {
            while(true) {
                init();

                // build interference graph
                buildIntGraph();

                // init work lists
                buildWorkLists();

                do {
                    if (!simplifyWorklist.empty())
                        simplify();
                    if (!workListMoves.empty())
                        coalesce();
                    if (!freezeWorklist.empty())
                        freeze();
                    if (!spillWorklist.empty())
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

    public:

        explicit ColourAllocator(Util::StackScheme &stack, IntermediateRepresentation::Function& func) :
        baseType::RegisterAllocator(stack, decltype(baseType::sourceFunc) (&func)) {
            // load pre-colour
            //
            /*
             * call %dest, func, %1, %2, %3, %4, %5, ..., %N
             *
             * %dest is pre-coloured: r0
             * %1, %2, %3, %4 = r0, r1, r2, r3
             * */
            auto& stmts = func.getStatements();

            for (auto& stmt : stmts) {
                auto& ops = stmt.getOps();
                const int opsCount = static_cast<int>(ops.size());
                if (stmt.getStmtType() == IntermediateRepresentation::CALL) {
                    // pre-colour %dest
                    if (ops[0].getIrOpType() == IntermediateRepresentation::Var) {
                        preColoured.insert(ops[0]);
                        preColourScheme[ops[0]] = 0;
                    }
                    for (int i = 2; i < std::min(6, opsCount); i++) {
                        if (ops[i].getIrOpType() == IntermediateRepresentation::Var) {
                            preColoured.insert(ops[i]);
                            // op1 -> r0, op2 -> r1, ...
                            preColourScheme[ops[i]] = i - 2;
                        }
                    }
                }
            }

            doFunctionScan();
            auto&& nodes = interferenceGraph.getNodes();
            baseType::variables = decltype(baseType::variables) { nodes.begin(), nodes.end() };
            for (const auto& var : baseType::variables)
                baseType::allocation[var] = colour[alias.find(var)];
        }

        ~ColourAllocator() = default;
    };

}

#endif //SYSYBACKEND_IRREGISTERALLOCATION_H
