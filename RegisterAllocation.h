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
        virtual void doFunctionScan() = 0;
    public:

        virtual ~RegisterAllocator() = 0;
        explicit RegisterAllocator(std::shared_ptr<IntermediateRepresentation::Function> func) : sourceFunc(std::move(func)) { };
        RegisterAllocator() = default;
        const auto& getAllocation() { return allocation; }
        const auto& getVariables() { return variables; }
    };

    template<size_t registerCount>
    class LinearScan : public RegisterAllocator<registerCount> {

    };

    template<size_t registerCount>
    class ColourAllocator : public RegisterAllocator<registerCount> {

        using baseType = RegisterAllocator<registerCount>;
        using bb_t = Flow::bb_ptr_t;
        using is_t = std::shared_ptr<Flow::BasicBlock::BBStatement>;
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
        std::list<var_t> simplifyWorklist;
        std::stack<var_t> selectStack;
        Util::DisjointSet<var_t> alias;

        std::set<is_t> nodeMoves(const var_t&);

        void enableMoves(const var_t&);

        void decrementDegree(const var_t&);

        void combine(const var_t& u, const var_t& v);

        void coalesce();

        void freeze();

        void freezeMoves(const var_t&);

        void simplify();

        void assignColours();

        void buildWorkLists();

        void selectSpill();

        void buildIntGraph();

        void rewriteFunction();

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
                selectStack.pop();
            coalescedMoves.clear();
            constrainedMoves.clear();
            frozenMoves.clear();
            activeMoves.clear();
            // build initial & alias
            alias = Util::DisjointSet<var_t> { interferenceGraph.getNodes() };
            initial = interferenceGraph.getNodes();
            initial = Util::set_diff(initial, preColoured);

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

        explicit ColourAllocator(IntermediateRepresentation::Function& func) :
        baseType::RegisterAllocator(decltype(baseType::sourceFunc) (&func)) {
            // load pre-colour
            auto& params = func.getParameters();
            for (auto& param : params)
                preColoured.insert(param);

            doFunctionScan();
            baseType::variables = interferenceGraph.getNodes();
            baseType::allocation = colour;
        }
    };

}

#endif //SYSYBACKEND_IRREGISTERALLOCATION_H
