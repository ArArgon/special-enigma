
#ifndef SYSYBACKEND_NOREGISTERALLOCATION_H
#define SYSYBACKEND_NOREGISTERALLOCATION_H

#include "RegisterAllocation.h"


namespace Backend::RegisterAllocation {

    template<size_t registerCount>
    class NoRegisterAllocation : public RegisterAllocator<registerCount> {

        using baseType = RegisterAllocator<registerCount>;
        using bb_t = Flow::bb_ptr_t;

        using is_t = Flow::BasicBlock::BBStatement*;
        using var_t = IntermediateRepresentation::IROperand;

        std::shared_ptr<Flow::Flow> flowAnalyzer = nullptr;
        std::vector<bb_t> basicBlocks;
        std::unordered_map<var_t, size_t> preColourScheme;
        std::unordered_set<var_t> preColoured;

        void saveFunction() {
            std::vector<IntermediateRepresentation::Statement> stmts;
            for (auto &bb : basicBlocks) {
                for (auto &ins : bb->statements) {
                    stmts.push_back(*ins.statement);
                }
            }
            baseType::sourceFunc->setStatements(stmts);
        }

        void doFunctionScan() override {
            while (true) {

                flowAnalyzer = std::make_shared<Flow::Flow>(Flow::Flow { baseType::sourceFunc });
                basicBlocks = flowAnalyzer->getBasicBlocks();

                rewriteFunction();
            }
        }

        void rewriteFunction() {

            for (auto &bb : basicBlocks) {
                auto &ins = bb->statements;
                for (auto it = ins.begin(); it != ins.end(); it++) {
                    for (auto &useVar : it->use) {
                        baseType::stackScheme->allocate(useVar);
                    }
                    for (auto &defVar : it->def) {
                        baseType::stackScheme->allocate(defVar);
                    }
                }
            }

            for (auto &bb : basicBlocks) {
                auto &ins = bb->statements;
                for (auto it = ins.begin(); it != ins.end(); it++) {
                    if (!it->init)
                        continue;
                    std::unordered_set<size_t> registerSet = {0, 1, 2, 3};
                    auto afterIt = it;
                    for (auto &useVar : it->use) {
                        if (preColourScheme.count(useVar)) {
                            registerSet.erase(preColourScheme[useVar]);
                            continue;
                        }
                        size_t offset = baseType::stackScheme->getVariablePosition(useVar);

                        auto tmpRegister = useVar;
                        tmpRegister.setVarName("r" + std::to_string(*registerSet.begin()));

                        // insert before
                        Flow::BasicBlock::BBStatement tmpStmt;
                        auto load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_LOAD,
                                                                             IntermediateRepresentation::i32,
                                                                             tmpRegister,
                                                                             IntermediateRepresentation::IROperand(
                                                                                     IntermediateRepresentation::i32,
                                                                                     offset));
                        tmpStmt.statement = new IntermediateRepresentation::Statement(load_st);
                        it = ins.insert(it, tmpStmt) + 1;

                        // insert after
                        load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_STR,
                                                                        IntermediateRepresentation::i32, tmpRegister,
                                                                        IntermediateRepresentation::IROperand(
                                                                                IntermediateRepresentation::i32,
                                                                                offset));
                        tmpStmt.statement = new IntermediateRepresentation::Statement(load_st);
                        afterIt = ins.insert(afterIt + 1, tmpStmt);

                    }

                    for (auto &defVar : it->def) {
                        if (preColourScheme.count(defVar)) {
                            registerSet.erase(preColourScheme[defVar]);
                            continue;
                        }
                        size_t offset = baseType::stackScheme->getVariablePosition(defVar);

                        auto tmpRegister = defVar;
                        tmpRegister.setVarName("r" + std::to_string(*registerSet.begin()));

                        // insert before
                        Flow::BasicBlock::BBStatement tmpStmt;
                        auto load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_LOAD,
                                                                             IntermediateRepresentation::i32,
                                                                             tmpRegister,
                                                                             IntermediateRepresentation::IROperand(
                                                                                     IntermediateRepresentation::i32,
                                                                                     offset));
                        tmpStmt.statement = new IntermediateRepresentation::Statement(load_st);
                        it = ins.insert(it, tmpStmt) + 1;

                        // insert after
                        load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_STR,
                                                                        IntermediateRepresentation::i32, tmpRegister,
                                                                        IntermediateRepresentation::IROperand(
                                                                                IntermediateRepresentation::i32,
                                                                                offset));
                        tmpStmt.statement = new IntermediateRepresentation::Statement(load_st);
                        afterIt = ins.insert(afterIt + 1, tmpStmt);

                    }
                }
            }
        }

    public:
        NoRegisterAllocation(Util::StackScheme * stack, IntermediateRepresentation::Function * func) :
            baseType::RegisterAllocator(stack, func) {
            // load pre-colour
            //
            /*
             * call %dest, func, %1, %2, %3, %4, %5, ..., %N
             *
             * %dest is pre-coloured: r0
             * %1, %2, %3, %4 = r0, r1, r2, r3
             * */
            auto& stmts = baseType::sourceFunc->getStatements();
            auto& params = baseType::sourceFunc->getParameters();

            // function parameters
            for (int i = 0; i < std::min(4, (int)params.size()); i++) {
                preColoured.insert(params[i]);
                preColourScheme[params[i]] = i;
            }

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
                else if (stmt.getStmtType() == IntermediateRepresentation::RETURN) {
                    if (stmt.getDataType() != IntermediateRepresentation::t_void && ops[0].getIrOpType() == IntermediateRepresentation::Var) {
                        // return   %xxx
                        preColoured.insert(ops[0]);
                        preColourScheme[ops[0]] = 0;
                    }
                }
            }
        };
    };
}

#endif //SYSYBACKEND_NOREGISTERALLOCATION_H