
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
            // load basic blocks and compute def, use information
            flowAnalyzer = std::make_shared<Flow::Flow>(Flow::Flow { baseType::sourceFunc });
            basicBlocks = flowAnalyzer->getBasicBlocks();

            // allocate stack space for every variable
            for (auto &bb : basicBlocks) {
                auto &ins = bb->statements;
                for (auto & in : ins) {
                    auto var_set = in.use;
                    Util::set_union_to(var_set, in.def);
                    for (const auto &var : var_set)
                        baseType::stackScheme->allocate(var);
                }
            }

            // save allocation

            for (int i = 0; i <= 5; i++) {
                var_t regI { IntermediateRepresentation::i32, "r" + std::to_string(i) };
                baseType::allocation[regI] = i;
                baseType::variables.insert(regI);
            }

            // rewrite function
            rewriteFunction();
        }

        void rewriteFunction() {
            // function-wide available registers
            // std::set is used to keep order.
            std::set<size_t> registerSet = { 4, 5 };
            if (baseType::sourceFunc->getParameters().empty()) {
                registerSet.insert(0);
                registerSet.insert(1);
                registerSet.insert(2);
                registerSet.insert(3);
            }

            std::map<var_t, int> varBind, callerStkOff;
            std::set<var_t> bindSet;
            for (auto &bb : basicBlocks) {
                auto &ins = bb->statements;
                for (auto it = ins.begin(); it != ins.end(); it++) {
                    if (!it->init)
                        continue;
                    auto regToOpr = [] (size_t num) {
                        return var_t { IntermediateRepresentation::i32, "r" + std::to_string(num) };
                    };
                    auto immOpr = [] (int num) {
                        return var_t {IntermediateRepresentation::i32, num };
                    };
                    auto getStackPos = [&] (const var_t& opr) {
                        return baseType::stackScheme->getVariablePosition(opr);
                    };
                    auto insertBefore = [&] (const IntermediateRepresentation::Statement& stmt) {
                        Flow::BasicBlock::BBStatement tmpBB;
                        tmpBB.statement = new IntermediateRepresentation::Statement(stmt);
                        it = ins.insert(it, tmpBB) + 1;
                    };
                    auto insertAfter = [&] (const IntermediateRepresentation::Statement& stmt) {
                        Flow::BasicBlock::BBStatement tmpBB;
                        tmpBB.statement = new IntermediateRepresentation::Statement(stmt);
                        it = ins.insert(it + 1, tmpBB) - 1;
                    };
                    if (it->statement->getStmtType() == IntermediateRepresentation::PARAM) {
                        auto param = it->statement->getOps()[0];
                        int pos = param.getValue();
                        if (it->statement->getOps().size() == 1) {
                            // Caller param:
                            // param        [%paramName, #pos]
                            // TODO: freeze & bind
                            /*
                             * for var in register: bind to its corresponding register
                             * for var in stack:    bind to -1
                             * */
                            bindSet.insert(param);
                            if (pos > 3 || pos < 0) {
                                // bind to -1
                                varBind[param] = -1;
                                callerStkOff[param] = pos;
                            } else {
                                varBind[param] = pos;
                                registerSet.erase(pos);
                            }
                        } else {
                            // Callee param:
                            // param        [%paramName, #pos], <placeholder>
                            // unfreeze     r#pos
                            if (pos < 4) {
                                // in register
                                // insert before
                                // stk_str      r#pos, #stk_pos
                                insertBefore({ IntermediateRepresentation::STK_STR, IntermediateRepresentation::i32, regToOpr(pos), immOpr(getStackPos(param)) });
                                registerSet.insert(pos);
                            } else {
                                // in stack
                                // stk_load     r<tmp>, #param_stk_pos, TODO: special placeholder indicate fp
                                // stk_str      r<tmp>, #stk_pos

                                // insert before
                                auto tmpReg = *registerSet.begin();
                                // unable to determine offset, use a placeholder to mark
                                insertBefore({ IntermediateRepresentation::STK_LOAD, IntermediateRepresentation::i32, regToOpr(tmpReg), immOpr(-pos) });
                                insertBefore({ IntermediateRepresentation::STK_STR, IntermediateRepresentation::i32, regToOpr(tmpReg), immOpr(getStackPos(param)) });
                            }
                        }
                        continue;
                    }
                    if (it->statement->getStmtType() == IntermediateRepresentation::CALL) {
                        /*
                         * unfreeze r1, r2, r3 ( if it is not mod func )
                         * unfreeze r0, r2, r3 ( if it is mod func )
                         *
                         * store dest
                         * unfreeze r0, r1
                         * */
                        std::string callFunc = it->statement->getOps()[1].getStrValue();
                        var_t dest = it->statement->getOps()[0];


                        // unbind
                        bindSet.clear();
                        varBind.clear();

                        // unfreeze
                        std::set<size_t> tmpSet = { 2, 3 };
                        Util::set_union_to(registerSet, tmpSet);

                        // load data
                        // insert after
                        if (it->statement->getOps()[0].getIrDataType() != IntermediateRepresentation::t_void) {
                            size_t resReg = (callFunc != "__aeabi_idivmod") ? 0 : 1;
                            insertAfter({ IntermediateRepresentation::STK_STR, IntermediateRepresentation::i32, regToOpr(resReg), immOpr(getStackPos(dest)) });
                        }
                        registerSet.insert(0);
                        registerSet.insert(1);
                        continue;
                    }
                    std::cout << "Modify statement: " << it->statement->toString() << std::endl;
                    auto itDef = it->def, itUse = it->use;
                    std::set<size_t> usedReg;
                    for (auto& defVar : itDef) {
                        size_t offset = baseType::stackScheme->getVariablePosition(defVar);

                        auto tmpRegister = defVar;
                        if (bindSet.count(tmpRegister) && varBind.at(tmpRegister) != -1)
                            tmpRegister.setVarName("r" + std::to_string(varBind[tmpRegister]));
                        else {
                            int reg = *registerSet.begin();
                            tmpRegister.setVarName("r" + std::to_string(reg));
                            registerSet.erase(reg);
                            usedReg.insert(reg);
                        }
                        tmpRegister.setIrType(IntermediateRepresentation::i32);

                        // replace variable
                        it->replaceDef(defVar, tmpRegister);
                        std::cout << "Replace def: " << it->statement->toString() << std::endl;

                        if (itUse.count(defVar)) {
                            it->replaceUse(defVar, tmpRegister);
                            std::cout << "Replace use & def: " << it->statement->toString() << std::endl;
                            itUse.erase(defVar);
                            Flow::BasicBlock::BBStatement tmpStmt;
                            auto load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_LOAD,
                                                                                 IntermediateRepresentation::i32,
                                                                                 tmpRegister,
                                                                                 immOpr(offset));
                            tmpStmt.statement = new IntermediateRepresentation::Statement(load_st);
                            it = ins.insert(it, tmpStmt) + 1;
                            std::cout << "Insert before def & use: " << tmpStmt.statement->toString() << std::endl;
                        }

                        IntermediateRepresentation::Statement load_st;

                        // insert after
                        load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_STR,
                                                                        IntermediateRepresentation::i32, tmpRegister,
                                                                        immOpr(offset));
                        insertAfter(load_st);
                        std::cout << "Insert after def: " << load_st.toString() << std::endl;

                        // push stk for caller
                        if (varBind[defVar] == -1) {
                            offset = callerStkOff[defVar] + 1;
                            load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_STR,
                                                                            IntermediateRepresentation::i32, tmpRegister,
                                                                            immOpr(-offset), var_t());
                            insertAfter(load_st);
                            std::cout << "Insert after def for caller: " << load_st.toString() << std::endl;
                        }
                    }
                    for (auto &useVar : itUse) {
                        size_t offset = baseType::stackScheme->getVariablePosition(useVar);

                        auto tmpRegister = useVar;
                        if (bindSet.count(tmpRegister) && varBind.at(tmpRegister) != -1)
                            tmpRegister.setVarName("r" + std::to_string(varBind[tmpRegister]));
                        else {
                            int reg = *registerSet.begin();
                            tmpRegister.setVarName("r" + std::to_string(reg));
                            registerSet.erase(reg);
                            usedReg.insert(reg);
                        }
                        tmpRegister.setIrType(IntermediateRepresentation::i32);

                        // replace use
                        it->replaceUse(useVar, tmpRegister);
                        std::cout << "Replace use: " << it->statement->toString() << std::endl;

                        // insert before
                        if (!itDef.count(useVar)) {
                            Flow::BasicBlock::BBStatement tmpStmt;
                            auto load_st = IntermediateRepresentation::Statement(IntermediateRepresentation::STK_LOAD,
                                                                                 IntermediateRepresentation::i32,
                                                                                 tmpRegister,
                                                                                 immOpr(offset));
                            tmpStmt.statement = new IntermediateRepresentation::Statement(load_st);
                            it = ins.insert(it, tmpStmt) + 1;
                            std::cout << "Insert before use: " << tmpStmt.statement->toString() << std::endl;
                        }
                    }

                    Util::set_union_to(registerSet, usedReg);
                }
            }
        }

    public:
        NoRegisterAllocation(Util::StackScheme * stack, IntermediateRepresentation::Function * func) :
        baseType::RegisterAllocator(stack, func) {
            auto& stmts = baseType::sourceFunc->getStatements();
            auto& params = baseType::sourceFunc->getParameters();

            doFunctionScan();
            saveFunction();
        };

        ~NoRegisterAllocation() = default;
    };
}

#endif //SYSYBACKEND_NOREGISTERALLOCATION_H