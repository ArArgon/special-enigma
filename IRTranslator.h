//
// Created by 廖治平 on 5/30/21.
//

#ifndef SYSYBACKEND_IRTRANSLATOR_H
#define SYSYBACKEND_IRTRANSLATOR_H

#include <unordered_map>
#include <memory>
#include <utility>
#include <stack>
#include <set>
#include <algorithm>
#include <functional>
#include <type_traits>

#include "Instruction.h"
#include "IRTypes.h"
#include "RegisterAllocation.h"
#include "InstructionUtilities.h"

namespace Backend::Translator {
    // Base class for translator
    using namespace Instruction::Utilities;
    using namespace Instruction::Utilities::Abbr;
    using namespace Instruction;

    constexpr size_t availableRegister = 9;
    bool enable_direct_label_write = false;

    class TranslatorBase {
    protected:
        IntermediateRepresentation::IRProgram irProgram;
        Instruction::InstructionStream ans;

    public:
        TranslatorBase() = default;
        virtual ~TranslatorBase() = default;
        explicit TranslatorBase(IntermediateRepresentation::IRProgram irProgram) : irProgram(std::move(irProgram)) { }
        virtual Instruction::InstructionStream doTranslation() = 0;

        virtual const Instruction::InstructionStream& getAns() { return ans; }
    };

    template<template<size_t> class Allocator, size_t registerCount>
    class Translator : public TranslatorBase {
        static_assert(std::is_base_of<RegisterAllocation::RegisterAllocator<registerCount>, Allocator<registerCount>>(),
                "Allocator must be a derived class of RegisterAllocation::RegisterAllocator");
        using ArmRegAllocator = RegisterAllocation::RegisterAllocator<registerCount>;
        using allocator_t = Allocator<registerCount>;
        std::unique_ptr<ArmRegAllocator> allocator;

        void procGlobal(InstructionStream& ins, std::unordered_map<std::string, std::string>& globalToLabel,
                        std::unordered_set<IntermediateRepresentation::IROperand>& globalSymbol) {
            auto& globalVar = irProgram.getGlobal();
            auto& globalArr = irProgram.getGlobalArrays();

            InstructionStream ptrIns, valIns;

            for (auto& var : globalVar) {
                /**
                 * global int
                 *
                 * global_xx    i32 %var, <int>
                 * global_xx    str %var, "<str>"
                 *
                 * __GLB_VAR_PTR_xxx:
                 *      .long   __GLB_VAR_xxx
                 * __GLB_VAR_xxx:
                 *      .long   <value>
                 *
                 *
                 * global string
                 * __GLB_STR_PTR_xxx:
                 *      .long   __GLB_STR_xxx
                 * __GLB_STR_xxx:
                 *      .asciz  "<value>"
                 * */
                 std::vector<IntermediateRepresentation::IROperand> ops = var.getOps();
                 std::string varName = ops[0].getVarName(), label_ptr, label_val;
                 if (ops[0].getIrDataType() == IntermediateRepresentation::i32) {
                     // global int
                     label_ptr = "__GLB_VAR_PTR_" + varName, label_val = "__GLB_VAR_" + varName;
                     ptrIns << LabelInstruction(label_ptr) << DotInstruction(Instruction::DotInstruction::LONG, label_val);
                     if (ops.size() > 1)
                        valIns << LabelInstruction(label_val) << DotInstruction(Instruction::DotInstruction::LONG, static_cast<uint32_t> (ops[1].getValue()), false);
                     else
                         valIns << LabelInstruction(label_val) << DotInstruction(Instruction::DotInstruction::LONG, static_cast<uint32_t> (0), false);
                 } else if (ops[0].getIrDataType() == IntermediateRepresentation::str){
                     // global string
                     ops[0].setIsPointer(true);
                     label_ptr = "__GLB_STR_PTR_" + varName, label_val = "__GLB_STR_" + varName;
                     if (ops.size() <= 1)
                         throw std::runtime_error("Invalid IR: global string definition must have a value");
                     ptrIns << LabelInstruction(label_ptr) << DotInstruction(Instruction::DotInstruction::LONG, label_val);
                     valIns << LabelInstruction(label_val) << DotInstruction(Instruction::DotInstruction::ASCIZ, ops[1].getStrValue());
                 }
                 globalToLabel[varName] = label_ptr;
                 globalSymbol.insert(ops[0]);
            }

            for (auto& arr : globalArr) {
                /**
                 * __GLB_ARR_PTR_xxx:
                 *      .long __GLB_ARR_xxx
                 *
                 * __GLB_ARR_xxx:
                 *      .zero <bytes>
                 *      .long <value>
                 * */
                size_t last_pos = 0, size = arr.getArrSize();
                std::string label_val = "__GLB_ARR_" + arr.getArrayName(), label_ptr = "__GLB_ARR_PTR_" + arr.getArrayName();
                globalToLabel[arr.getArrayName()] = label_ptr;
                globalSymbol.insert({ IntermediateRepresentation::i32, arr.getArrayName(), true });
                ptrIns << LabelInstruction(label_ptr) << DotInstruction(Instruction::DotInstruction::LONG, label_val);
                valIns << LabelInstruction(label_val);
                auto& dataMap = arr.getData();
                for (auto& data: dataMap) {
                    auto& pos = data.first;
                    auto& value = data.second;
                    if (pos - last_pos > 1)
                        valIns << DotInstruction(DotInstruction::ZERO, (pos - last_pos) * 4, false);
                    valIns << DotInstruction(DotInstruction::WORD, static_cast<uint32_t>(value), false);
                    last_pos = pos;
                }
                if (last_pos != size - 1)
                    valIns << DotInstruction(Instruction::DotInstruction::ZERO, (size - last_pos - 1) * 4, false);
            }
            ins.insert(ins.end(), ptrIns.begin(), ptrIns.end());
            ins.insert(ins.end(), valIns.begin(), valIns.end());
        }

        void preProcFunc(IntermediateRepresentation::Function &func, Util::StackScheme& stackScheme,
                         const std::unordered_map<std::string, std::string>& globalMapping,
                         const std::unordered_set<IntermediateRepresentation::IROperand>& globalSymbols) {
            auto &stmts = func.getRefStatements();
            auto params = func.getParameters();

            /*
             * param    [%param_dest, #pos], [%param_dest, #pos], ...
             * mov      %dest,  %param
             * */
            if (!params.empty()) {
                int pos = params.size() - 1;
                auto st_it = stmts.begin();
                std::vector<IntermediateRepresentation::IROperand> paramTmp;
                for (auto rit = params.rbegin(); rit != params.rend(); rit++) {
                    auto tmpOpr = *rit;
                    rit->setVarName("param_" + tmpOpr.getVarName());
                    auto param = *rit;
                    param.setValue(pos--);
                    paramTmp.push_back(param);
                    st_it = stmts.insert(st_it, { IntermediateRepresentation::MOV, IntermediateRepresentation::i32, tmpOpr, *rit } ) + 1;
                }

                stmts.insert(stmts.begin(), { IntermediateRepresentation::PARAM, IntermediateRepresentation::i32, paramTmp } );

            }

            func.setParameters(params);

            for (auto& sym : globalSymbols) {
                int occurrence = 0;
                std::string ptrNamePrefix = "glb_ptr_" + sym.getVarName(), valName = "glb_val_" + sym.getVarName();
                IntermediateRepresentation::IROperand valOpr(sym);
                valOpr.setVarName(valName);

                for (auto it = stmts.begin(); it != stmts.end(); it++) {
                    auto& ops = it->getRefOps();
                    auto analysis = Flow::BasicBlock::procRawStatement(*it);
                    IntermediateRepresentation::IROperand ptrOpr(sym);
                    ptrOpr.setVarName(ptrNamePrefix + "_" + std::to_string(occurrence));
                    if (analysis.def.count(sym) + analysis.use.count(sym)) {
                        // generate register IR before 'it'
                        // GLOBAL_XXX       glb_ptr_<varName>_<occurrence>   "<varName>"
                        auto regGlobal = IntermediateRepresentation::Statement(
                                sym.getIsPointer() ? IntermediateRepresentation::GLB_ARR : IntermediateRepresentation::GLB_VAR,
                                IntermediateRepresentation::i32, ptrOpr, IntermediateRepresentation::IROperand(sym.getVarName())
                        );
                        it = stmts.insert(it, regGlobal) + 1;
                        ++occurrence;
                    }
                    if (analysis.def.count(sym)) {
                        if (sym.getIsPointer())
                            throw std::runtime_error("Invalid IR: global ptr cannot be changed, entailed IR: " + it->toString());

                        if (it->getStmtType() == IntermediateRepresentation::MOV) {
                            // mov      %<varName>, %xxx
                            // store    %xxx, %glb_ptr_<varName>, 0
                            *it = { IntermediateRepresentation::STORE, IntermediateRepresentation::i32, ops[1], ptrOpr, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32,0) };
                        } else {
                            // ins      %<varName>, %xxx, ...
                            // ins      %glb_val_<varName>, %xxx, ...

                            // insert before
                            // load     %glb_val_<varName>, %glb_ptr_<varName>, 0

                            it = stmts.insert(it, { IntermediateRepresentation::LOAD, IntermediateRepresentation::i32, valOpr, ptrOpr, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, 0) } ) + 1;

                            // insert after
                            // store    %glb_val_<varName>, %glb_ptr_<varName>, 0
                            it = stmts.insert(it + 1, { IntermediateRepresentation::STORE, IntermediateRepresentation::i32, valOpr, ptrOpr, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, 0) } ) - 1;

                            analysis.replaceDef(sym, valOpr);
                            analysis.replaceUse(sym, valOpr);
                        }
                    } else if (analysis.use.count(sym)) {
                        if (sym.getIsPointer())
                            analysis.replaceUse(sym, ptrOpr);
                        else {
                            // insert before
                            // load     %glb_val_<varName>, %glb_ptr_<varName>, 0
                            it = stmts.insert(it, { IntermediateRepresentation::LOAD, IntermediateRepresentation::i32, valOpr, ptrOpr, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, 0) } ) + 1;

                            analysis.replaceUse(sym, valOpr);
                        }
                    }
                }
            }

            for (auto it = stmts.begin(); it != stmts.end(); it++) {
                auto& stmt = *it;
                auto ops = stmt.getRefOps();
                switch (stmt.getStmtType()) {
                    case IntermediateRepresentation::MOD: {
                        // mod      %dest, %opr1, %opr2
                        // call     %dest, __aeabi_idivmod, %opr1, %opr2
                        stmt.setStmtType(IntermediateRepresentation::CALL);
                        stmt.setOps( { ops[0], IntermediateRepresentation::IROperand("__aeabi_idivmod"), ops[1], ops[2] } );
                    }
                        break;
                    case IntermediateRepresentation::DIV: {
                        // div      %dest, %opr1, %opr2
                        // call     %dest, __aeabi_idiv, %opr1, %opr2
                        stmt.setStmtType(IntermediateRepresentation::CALL);
                        stmt.setOps( { ops[0], IntermediateRepresentation::IROperand("__aeabi_idiv"), ops[1], ops[2] } );
                    }
                        break;
                    case IntermediateRepresentation::ALLOCA: {
                        /*
                         * effect: %dest contains the beginning address of this space
                         * alloca       *i32 %dest, i32 %size
                         *
                         * alloca       *i32 %dest, i32 %<stackPosition>
                         * */
                        if (ops[1].getIrOpType() == IntermediateRepresentation::Var)
                            throw std::runtime_error("Invalid IR: dynamic allocation on stack is prohibited. Entailed IR: " + stmt.toString());
                        int size = ops[1].getValue();
                        // alloca       *i32 %dest, i32 %<stackPosition>
                        ops[1].setValue(static_cast<int>(stackScheme.allocate(ops[0], size)));
                    }
                        break;
                    case IntermediateRepresentation::RETURN: {
                        if (stmt.getDataType() != IntermediateRepresentation::t_void && ops[0].getIrOpType() == IntermediateRepresentation::Var) {
                            /*
                             * return       %<ret>
                             *
                             * mov          %<func>_ret_<ret>, %ret
                             * return       %<func>_ret_<ret>
                             * */
                            auto tmpOps = ops[0];
                            tmpOps.setVarName("ret_" + ops[0].getVarName());
                            stmt.setOps( { tmpOps });
                            it = stmts.insert(it, { IntermediateRepresentation::MOV, IntermediateRepresentation::i32, tmpOps, ops[0] }) + 1;
                        }
                    }
                    default:
                        break;
                }
            }

            // rewrite function
            for (auto it = stmts.begin(); it != stmts.end(); it++) {
                auto& stmt = *it;
                auto ops = stmt.getRefOps();
                if (stmt.getStmtType() == IntermediateRepresentation::CALL) {
                    // function call rewrite
                    /*
                     * call     %dest, func, %1, %2, %3, %4, %5, ..., %N
                     *
                     * # generate alias
                     * mov      %<funcName>_arg_%1, %1
                     * mov      %<funcName>_arg_%2, %2
                     * mov      %<funcName>_arg_%3, %3
                     * ...
                     * mov      %<funcName>_arg_%N, %N
                     *
                     * # prepare parameters on the stack
                     * param    %<funcName>_arg_%5, -1
                     * param    %<funcName>_arg_%6, -2
                     * param    %<funcName>_arg_%7, -3
                     * ...
                     * param    %<funcName>_arg_%N, -(N-4)
                     *
                     * call     %<funcName>_dst_%dest, func, %<funcName>_arg_%1, %<funcName>_arg_%2, ..., %<funcName>_arg_%N
                     * mov      %dest, %<funcName>_dst_%dest
                     * */

                    /*
                     * call     null, func, %1
                     *
                     * Placeholders are needed
                     *
                     * mov      %func_arg_%1, %1
                     *
                     * param    %func_arg_placeholder_1, null
                     * param    %func_arg_placeholder_2, null
                     *
                     * call     null, func, %1, %func_arg_placeholder_1, %func_arg_placeholder_2
                     *
                     * */
                    int paramCount = static_cast<int>(ops.size()) - 2;
                    std::string funcName = ops[1].getStrValue();
                    std::vector<IntermediateRepresentation::IROperand> replaceList;
                    auto replaceDest = ops[0];
                    if (ops[0].getIrOpType() == IntermediateRepresentation::Var)
                        replaceDest.setVarName(funcName + "_dst_" + ops[0].getVarName());
                    replaceList.push_back(replaceDest);
                    replaceList.push_back(ops[1]);

                    std::vector<IntermediateRepresentation::IROperand> paramOpr;
                    for (int i = 1 + 1; i <= paramCount + 1; i++) {
                        // prepare parameters
                        // param    [%<funcName>_arg_%x, -(x-4)]
                        auto tmpOpr = IntermediateRepresentation::IROperand(IntermediateRepresentation::t_void, "");
                        if (ops[i].getIrOpType() == IntermediateRepresentation::Var)
                            tmpOpr.setVarName(funcName + "_arg_" + ops[i].getVarName());
                        else
                            tmpOpr.setVarName(funcName + "_arg_imm_" + std::to_string(i));
                        if (i >= 6) {
                            tmpOpr.setIrType(IntermediateRepresentation::i32);
                            tmpOpr.setValue(-(i - 5));
                        }
                        paramOpr.push_back(tmpOpr);
                    }
                    // insert placeholders
                    if (paramCount < 4) {
                        for (int i = paramCount + 1; i <= 4; i++) {
                            auto tmpOpr = IntermediateRepresentation::IROperand(IntermediateRepresentation::t_void, "");
                            tmpOpr.setVarName(funcName + "_placeholder_" + std::to_string(i - 1));
                            replaceList.push_back(tmpOpr);
                            // param        [ %<func>_arg_placeholder_x, null ]
                            paramOpr.push_back(tmpOpr);
                        }
                    }
                    it = stmts.insert(it, { IntermediateRepresentation::PARAM, IntermediateRepresentation::i32, paramOpr }) + 1;

                    if (paramCount > 4) {
                        for (int i = 6; i <= paramCount + 1; i++) {
                            // insert before
                            // stk_str      %replaceList[i], #paramOpr[i].getValue()
                            it = stmts.insert(it, {
                                IntermediateRepresentation::STK_STR, IntermediateRepresentation::i32, replaceList[i],
                                IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, paramOpr[i].getValue())
                            }) + 1;
                        }
                    }

                    // generate alias
                    for (int i = 1 + 1; i <= paramCount + 1; i++) {
                        auto tmpOpr = IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, "");
                        if (ops[i].getIrOpType() == IntermediateRepresentation::Var)
                            tmpOpr.setVarName(funcName + "_arg_" + ops[i].getVarName());
                        else
                            tmpOpr.setVarName(funcName + "_arg_imm_" + std::to_string(i));
                        replaceList.push_back(tmpOpr);
                        // mov      %<funcName>_arg_%x, %x
                        it = stmts.insert(it, { IntermediateRepresentation::MOV, IntermediateRepresentation::i32, tmpOpr, ops[i] } ) + 1;
                    }

                    // replace function parameters
                    it->setOps(replaceList);

                    // save return
                    // mov      %dest, %<funcName>_dst_%dest
                    if (ops[0].getIrOpType() == IntermediateRepresentation::Var)
                        it = stmts.insert(it + 1, { IntermediateRepresentation::MOV, IntermediateRepresentation::i32, ops[0], replaceDest} ) - 1;
                }
            }

            // safe return
            if (stmts.rbegin()->getStmtType() != IntermediateRepresentation::RETURN) {
                // append return
                if (func.getReturnType() == IntermediateRepresentation::t_void) {
                    // return;
                    stmts.emplace_back(IntermediateRepresentation::RETURN, IntermediateRepresentation::t_void, IntermediateRepresentation::IROperand());
                }
                else if (func.getReturnType() == IntermediateRepresentation::t_void) {
                    // return 0;
                    stmts.emplace_back(IntermediateRepresentation::RETURN, IntermediateRepresentation::i32, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, 0));
                }
            }

        }

    public:
        Translator() = default;
        ~Translator() = default;
        explicit Translator(IntermediateRepresentation::IRProgram irProgram) : TranslatorBase(std::move(irProgram)) { }

        Instruction::InstructionStream doTranslation() override {
            InstructionStream ins;
            // TODO
            auto functions = irProgram.getFunctions();
            std::unordered_map<std::string, std::string> globalMapping;
            std::unordered_set<IntermediateRepresentation::IROperand> globalSymbols;
            auto loadImm = [&] (int imm) {
                int pos = imm < 0 ? ~imm : imm;
                ins << MoveInstruction(r9, imm16(0xffff & pos), false, Instruction::MoveInstruction::LOW)
                    << MoveInstruction(r9, imm16(pos >> 16), false, Instruction::MoveInstruction::HIGH);
                if (imm < 0)
                    ins << MoveInverseInstruction(r9, r9);
            };

            // globalIns
            procGlobal(ins, globalMapping, globalSymbols);

            for(auto& func : functions) {
                auto stackLayout = Util::StackScheme { };

                /* preprocess function
                 * rewrite  mod
                 * rewrite  div
                 * replace  globals
                 * allocate space
                 * */
                preProcFunc(func, stackLayout, globalMapping, globalSymbols);

                std::cout << "After preprocessing: " << std::endl << func.toString() << std::endl;

                allocator = std::make_unique<allocator_t>(allocator_t (&stackLayout, &func));

                auto allocation = allocator->getAllocation();
                auto variables = allocator->getVariables();
                auto totalColours = allocator->getTotalColours();
                const std::vector<IntermediateRepresentation::Statement>& stmts = func.getStatements();

                std::cout << "Translator: Register assignment complete" << std::endl;
                std::cout << "After register assignment: " << std::endl << func.toString() << std::endl;

                std::cout << "Allocation scheme: " << std::endl;
                for (auto& alloc : allocation) {
                    std::cout << "\t" << alloc.first.toString() << ": " << numToReg[alloc.second] << std::endl;
                }

                // mapping colours
//                std::list<Operands::Register> remainRegisters;
//                std::unordered_map<size_t, Operands::Register> colourScheme;
                std::unordered_map<IntermediateRepresentation::IROperand, Operands::Register> mapping;
                Operands::RegisterList list;
                size_t pushSize = 0;

                list.emplaceRegister(r4, r5, r6, r7, r8, r9);

//                remainRegisters.emplace_back(r0, r1, r2, r3, r4, r5, r6, r7, r8);
//                for (auto colour : totalColours) {
//                    colourScheme[colour] = remainRegisters.front();
//                    list.insertRegister(remainRegisters.front());
//                    remainRegisters.pop_front();
//                }

                for (auto& var : variables)
                    if (allocation.count(var))
                        mapping[var] = numToReg[allocation.at(var)];

                /*
                 * function init
                 * */
                // <funcName>:
                ins << LabelInstruction(func.getFunName());
                // push { rx, rx, ..., fp, lr }
                list.emplaceRegister(fp, lr);
                ins << PushInstruction(list);
                pushSize = list.getRegList().size();
                // mov fp, sp
                ins << MoveInstruction(fp, sp);
                // sub sp, sp, #<stack_size>
                size_t stackSize = stackLayout.getStackSize() + [&] () {
                    // get maximum function parameter count
                    /*
                     * call     %dest, func, %1, %2, %3, %4, %5, ..., %n
                     * */
                    int ans = 0;
                    for (auto &stmt : stmts) {
                        if (stmt.getStmtType() == IntermediateRepresentation::CALL)
                            ans = std::max(ans, std::max(0, ((int) stmt.getOps().size()) - 6));
                    }
                    return ans;
                } () * 4;
                for (size_t remainStackSize = stackSize; remainStackSize; ) {
                    int sub = (int) std::min(remainStackSize, (size_t) 4095);
                    ins << SubtractionInstruction(sp, sp, imm12(sub));
                    remainStackSize -= sub;
                }

                for (auto& stmt : stmts) {
                    const auto& ops = stmt.getOps();
                    switch (stmt.getStmtType()) {
                        case IntermediateRepresentation::BR: {
                            if (ops.size() == 1) {
                                // b %label
                                ins << BranchInstruction(B, ops[0].getStrValue());
                            } else {
                                /*
                                 * br       %cond, lb1, lb2
                                 *
                                 * cmp      %cond, 1
                                 * breq     lb1
                                 * br       lb2
                                 * */
                                ins << ComparisonInstruction(CMP, mapping.at(ops[0]), Operands::Operand2(imm8(1)));
                                auto breq = BranchInstruction(B, ops[1].getStrValue());
                                breq.setCondition(Instruction::Condition::Cond_Equal);
                                ins << std::move(breq);
                                ins << BranchInstruction(B, ops[2].getStrValue());
                            }
                        }
                            break;
                        case IntermediateRepresentation::ADD: {
                            /*
                             * add i32 %dest, i32 %opr1, i32 %opr2
                             * */
#warning "Imm12 not implemented"
                            auto dest = mapping.at(ops[0]), opr1 = mapping.at(ops[1]);
                            if (ops[2].getIrOpType() == IntermediateRepresentation::Var)
                                ins << AdditionInstruction(dest, opr1, mapping.at(ops[2]));
                            else
                                ins << AdditionInstruction(dest, opr1, imm12(ops[2].getValue()));
                        }
                            break;
                        case IntermediateRepresentation::MUL: {
#warning "Imm12 not implemented"
                            auto dest = mapping.at(ops[0]), opr1 = mapping.at(ops[1]);
                            if (ops[2].getIrOpType() == IntermediateRepresentation::Var)
                                ins << MultiplicationInstruction(dest, opr1, mapping.at(ops[2]));
                            else
                                ins << MultiplicationInstruction(dest, opr1, imm12(ops[2].getValue()));
                        }
                            break;
                        case IntermediateRepresentation::SUB: {
#warning "Imm12 not implemented"
                            auto dest = mapping.at(ops[0]), opr1 = mapping.at(ops[1]);
                            if (ops[2].getIrOpType() == IntermediateRepresentation::Var)
                                ins << SubtractionInstruction(dest, opr1, mapping.at(ops[2]));
                            else
                                ins << SubtractionInstruction(dest, opr1, imm12(ops[2].getValue()));
                        }
                            break;
                        case IntermediateRepresentation::MOD:
                        case IntermediateRepresentation::DIV:
                            // these should not appear here
                            throw std::invalid_argument("mod or sub has been preprocessed and should not appear in translation");
                            break;
                        case IntermediateRepresentation::PARAM: {
                            // this is a placeholder
                            break;
                            for (auto& opr : ops) {
                                if (opr.getIrDataType() == IntermediateRepresentation::t_void)
                                    continue;
                                int pos = opr.getValue();
                                if (pos >= 0) {
                                    if (pos <= 3) {
                                        if (numToReg[pos] != mapping.at(opr))
                                            ins << MoveInstruction(mapping.at(opr), numToReg[pos]);
                                    } else {
                                        // TODO calculate offset
                                        ins << LoadInstruction(mapping.at(opr), Operands::LoadSaveOperand(fp, 4 * pushSize + (pos - 5) * 4, true));
                                    }
                                } else {
                                    /*
                                     * When pos is negative, it means PARAM is for caller
                                     * param    %param, -1
                                     *
                                     * str      %param, [sp, #((-pos - 1) * 4)]
                                     * */
                                    ins << SaveInstruction(mapping.at(opr), Operands::LoadSaveOperand(sp, (-pos - 1) * 4, true));
                                }
                            }
                        }
                            break;
                        case IntermediateRepresentation::CALL: {
                            // prelude
                            /*
                             * call %dest, func, %1, %2, %3, %4, %5, ..., %N
                             *
                             * %1, %2, %3, %4:  has already assigned to registers
                             * %5, %6, ..., %N: has stored into stack
                             * %dest:           has prepared
                             *
                             * */
                            ins << BranchInstruction(BL, ops[1].getStrValue());
                        }
                            break;
                        case IntermediateRepresentation::RETURN: {
                            // mov      r0, ?
                            if (func.getReturnType() != IntermediateRepresentation::t_void) {
                                if (ops[0].getIrOpType() == IntermediateRepresentation::Var)
                                    if (mapping.at(ops[0]) != r0)
                                        ins << MoveInstruction(r0, mapping.at(ops[0]));
                                else {
#warning "Imm16 is not implemented"
                                    ins << MoveInstruction(r0, imm16(ops[0].getValue()));
                                }
                            }

                            // add      sp, sp #stack_size
                            for (size_t remainStackSize = stackSize; remainStackSize; ) {
                                int sub = (int) std::min(remainStackSize, (size_t) 4095);
                                ins << AdditionInstruction(sp, sp, imm12(sub));
                                remainStackSize -= sub;
                            }
                            // pop      { rx, rx, ..., fp, lr }
                            ins << PopInstruction(list);
                            // bx       lr
                            ins << BranchInstruction(BX, lr);
                        }
                            break;
                        case IntermediateRepresentation::MOV: {
                            // mov      %dest, %source
                            if (ops[1].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                ins << MoveInstruction(mapping.at(ops[0]), imm16(ops[1].getValue()));
                            } else {
                                if (mapping.at(ops[0]) != mapping.at(ops[1]))
                                    ins << MoveInstruction(mapping.at(ops[0]), mapping.at(ops[1]));
                            }
                        }
                            break;
                        case IntermediateRepresentation::LABEL: {
                            ins << LabelInstruction(ops[0].getStrValue());
                        }
                            break;
                        case IntermediateRepresentation::ALLOCA: {
                            /*
                             * alloca       %dest, imm %offset
                             *
                             * mov          %dest, #offset
                             * add          %dest, %dest, %sp
                             * */
#warning "Imm16 not implemented"
                            ins << MoveInstruction(mapping.at(ops[0]), imm16(stackSize - ops[1].getValue()));
                            ins << AdditionInstruction(mapping.at(ops[0]), mapping.at(ops[0]), sp);
                        }
                            break;
                        case IntermediateRepresentation::STK_LOAD: {
                            /*
                             * stk_load     i32 %dest, %off
                             *
                             * ldr          %dest, [lr, #off]
                             * */
                            ins << LoadInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(lr, ops[1].getValue(),
                                                                                              true));
                        }
                            break;
                        case IntermediateRepresentation::STK_STR: {
                            /*
                             * stk_str      i32 %src, %off
                             *
                             * str          %src, [lr, #off]
                             * */
                            if (ops.size() == 2)
                                ins << SaveInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(lr, ops[1].getValue(),true));
                            else {
                                // that's for function

                            }
                        }
                            break;
                        case IntermediateRepresentation::LOAD: {
                            /*
                             * load         i32 %dest, *i32 %base, i32 %off
                             *
                             * ldr          %dest, [%base, #off]
                             * */
                            ins << LoadInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(mapping.at(ops[1]), ops[2].getValue(), true));
                        }
                            break;
                        case IntermediateRepresentation::STORE: {
                            /*
                             * store        i32 %source, *i32 %base, i32 %off
                             *
                             * str          %source, [%base, #off]
                             * */
                            ins << SaveInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(mapping.at(ops[1]), ops[2].getValue(), true));
                        }
                            break;
                        case IntermediateRepresentation::CMP_EQ: {
                            /*
                             * cmp_xx       %dest, %opr1, %opr2
                             *
                             * cmp          %opr1, %opr2
                             * moveq        %dest, #1
                             * */
#warning "Imm8 is not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(ops[2].getValue())));
                            } else {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(mapping.at(ops[2])));
                            }
#warning "Imm16 is not implemented"
                            auto moveq = MoveInstruction(mapping.at(ops[0]), imm16(1));
                            moveq.setCondition(Instruction::Condition::Cond_Equal);
                            ins << std::move(moveq);
                        }
                            break;
                        case IntermediateRepresentation::CMP_NE: {
#warning "Imm8 is not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(ops[2].getValue())));
                            } else {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(mapping.at(ops[2])));
                            }
#warning "Imm16 is not implemented"
                            auto moveq = MoveInstruction(mapping.at(ops[0]), imm16(1));
                            moveq.setCondition(Instruction::Condition::Cond_NotEqual);
                            ins << std::move(moveq);
                        }
                            break;
                        case IntermediateRepresentation::CMP_SGE: {
#warning "Imm8 is not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(ops[2].getValue())));
                            } else {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(mapping.at(ops[2])));
                            }
#warning "Imm16 is not implemented"
                            auto moveq = MoveInstruction(mapping.at(ops[0]), imm16(1));
                            moveq.setCondition(Instruction::Condition::Cond_SGreaterEqual);
                            ins << std::move(moveq);
                        }
                            break;
                        case IntermediateRepresentation::CMP_SLE: {
#warning "Imm8 is not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(ops[2].getValue())));
                            } else {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(mapping.at(ops[2])));
                            }
#warning "Imm16 is not implemented"
                            auto moveq = MoveInstruction(mapping.at(ops[0]), imm16(1));
                            moveq.setCondition(Instruction::Condition::Cond_SLessEqual);
                            ins << std::move(moveq);
                        }
                            break;
                        case IntermediateRepresentation::CMP_SGT: {
#warning "Imm8 is not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(ops[2].getValue())));
                            } else {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(mapping.at(ops[2])));
                            }
#warning "Imm16 is not implemented"
                            auto moveq = MoveInstruction(mapping.at(ops[0]), imm16(1));
                            moveq.setCondition(Instruction::Condition::Cond_SGreater);
                            ins << std::move(moveq);
                        }
                            break;
                        case IntermediateRepresentation::CMP_SLT: {
#warning "Imm8 is not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(ops[2].getValue())));
                            } else {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(mapping.at(ops[2])));
                            }
#warning "Imm16 is not implemented"
                            auto moveq = MoveInstruction(mapping.at(ops[0]), imm16(1));
                            moveq.setCondition(Instruction::Condition::Cond_SLess);
                            ins << std::move(moveq);
                        }
                            break;
                        case IntermediateRepresentation::GLB_ARR:
                        case IntermediateRepresentation::GLB_CONST:
                        case IntermediateRepresentation::GLB_VAR: {
                            /*
                             * Global variables have modified
                             *
                             * glb_xxx      %var_pos, "<global name>"
                             * */
                            ins << LoadInstruction(mapping.at(ops[0]), globalMapping[ops[1].getStrValue()]);
                        }
                            break;
                        case IntermediateRepresentation::LSH: {
                            /*
                             * lsh      %dest, %src, %num
                             *
                             * lsl      %dest, %src, %num
                             * */
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal)
                                ins << ShiftInstruction(LSL, mapping.at(ops[0]), mapping.at(ops[1]), ops[2].getValue());
                            else
                                ins << ShiftInstruction(LSL, mapping.at(ops[0]), mapping.at(ops[1]), mapping.at(ops[2]));
                        }
                            break;
                        case IntermediateRepresentation::RSH: {
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal)
                                ins << ShiftInstruction(LSR, mapping.at(ops[0]), mapping.at(ops[1]), ops[2].getValue());
                            else
                                ins << ShiftInstruction(LSR, mapping.at(ops[0]), mapping.at(ops[1]), mapping.at(ops[2]));
                        }
                            break;
                        case IntermediateRepresentation::OR: {
                            /*
                             * or       %dest, %opr1, %opr2
                             *
                             * orr      %dest, %opr1, %opr2
                             * */
#warning "Imm8 not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                ins << BitInstruction(ORR, mapping.at(ops[0]), mapping.at(ops[1]), imm8(ops[2].getValue()));
                            } else {
                                ins << BitInstruction(ORR, mapping.at(ops[0]), mapping.at(ops[1]), mapping.at(ops[2]));
                            }
                        }
                            break;
                        case IntermediateRepresentation::AND: {
#warning "Imm8 not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                ins << BitInstruction(AND, mapping.at(ops[0]), mapping.at(ops[1]), imm8(ops[2].getValue()));
                            } else {
                                ins << BitInstruction(AND, mapping.at(ops[0]), mapping.at(ops[1]), mapping.at(ops[2]));
                            }
                        }
                            break;
                        case IntermediateRepresentation::XOR: {
#warning "Imm8 not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                ins << BitInstruction(EOR, mapping.at(ops[0]), mapping.at(ops[1]), imm8(ops[2].getValue()));
                            } else {
                                ins << BitInstruction(EOR, mapping.at(ops[0]), mapping.at(ops[1]), mapping.at(ops[2]));
                            }
                        }
                            break;
                        case IntermediateRepresentation::NOT: {
                            /*
                             * not      %dest
                             * mvn      %dest, %dest
                             * */
                            ins << MoveInverseInstruction(mapping.at(ops[0]), mapping.at(ops[0]));
                        }
                            break;
                        case IntermediateRepresentation::PHI:
                            throw std::runtime_error("Invalid IR: Phi function in SSA should not appear in backend IR. Entailed IR: " + stmt.toString());
                    }
                }

                // TODO Imm Fix
            }
            return ins;
        }
    };
}

#endif //SYSYBACKEND_IRTRANSLATOR_H
