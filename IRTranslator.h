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

extern bool isDebug;

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
        std::unordered_map<std::string, std::string> globalPtrToVal;

        void procGlobal(InstructionStream& textIns, InstructionStream& dataIns, std::unordered_map<std::string, std::string>& globalToLabel,
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
                 globalPtrToVal[label_ptr] = label_val;
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
                globalPtrToVal[label_ptr] = label_val;
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
            textIns.insert(textIns.end(), ptrIns.begin(), ptrIns.end());
            dataIns.insert(dataIns.end(), valIns.begin(), valIns.end());
        }

        void preProcFunc(IntermediateRepresentation::Function &func, Util::StackScheme& stackScheme,
                         const std::unordered_map<std::string, std::string>& globalMapping,
                         const std::unordered_set<IntermediateRepresentation::IROperand>& globalSymbols) {
            auto &stmts = func.getRefStatements();
            auto params = func.getParameters();

            /*
             * param    [%param_xx1, #pos], <placeholder>
             * mov      %xx1, %param_xx1
             * param    [%param_xx2, #pos], <placeholder>
             * mov      %xx2, %param_xx2
             * */
            if (!params.empty()) {
                int pos = params.size() - 1;
                auto st_it = stmts.begin();
                for (auto rit = params.rbegin(); rit != params.rend(); rit++) {
                    auto tmpOpr = *rit;
                    rit->setVarName("param_" + tmpOpr.getVarName());
                    auto param = *rit;
                    param.setValue(pos--);
                    st_it = stmts.insert(st_it, { IntermediateRepresentation::PARAM, IntermediateRepresentation::i32, param, IntermediateRepresentation::IROperand() }) + 1;
                    st_it = stmts.insert(st_it, { IntermediateRepresentation::MOV, IntermediateRepresentation::i32, tmpOpr, *rit } ) + 1;
                }
            }

            func.setParameters(params);

            for (auto& sym : globalSymbols) {
                int occurrence = 0;
                std::string ptrNamePrefix = "glb_ptr_" + sym.getVarName(), valName = "glb_val_" + sym.getVarName();
                IntermediateRepresentation::IROperand valOpr(sym);
                valOpr.setVarName(valName);

                for (auto it = stmts.begin(); it != stmts.end(); it++) {
//                    auto& ops = it->getRefOps();
                    if (it->getStmtType() == IntermediateRepresentation::GLB_VAR ||
                        it->getStmtType() == IntermediateRepresentation::GLB_ARR ||
                        it->getStmtType() == IntermediateRepresentation::GLB_CONST)
                        continue;
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
                        analysis = Flow::BasicBlock::procRawStatement(*it);
                        ++occurrence;
                    }
                    if (analysis.def.count(sym)) {
                        if (sym.getIsPointer())
                            throw std::runtime_error("Invalid IR: global ptr cannot be changed, entailed IR: " + it->toString());

                        if (it->getStmtType() == IntermediateRepresentation::MOV) {
                            // mov      %<varName>, %xxx
                            // store    %xxx, %glb_ptr_<varName>, 0
                            IntermediateRepresentation::IROperand sourceVal = it->getOps()[1];
                            if (it->getOps()[1].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                // insert before
                                /*
                                 * mov      %<varName>_tmp_<occurrence>, %source
                                 * */
                                IntermediateRepresentation::IROperand tmpSource { IntermediateRepresentation::i32, sym.getVarName() + "_tmp_" + std::to_string(occurrence) };
                                it = stmts.insert(it, { IntermediateRepresentation::MOV, IntermediateRepresentation::i32, tmpSource, sourceVal } ) + 1;
                                sourceVal = tmpSource;
                            }
                            *it = { IntermediateRepresentation::STORE, IntermediateRepresentation::i32, sourceVal, ptrOpr, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32,0) };
                        } else {
                            // ins      %<varName>, %xxx, ...
                            // ins      %glb_val_<varName>, %xxx, ...
                            analysis.replaceDef(sym, valOpr);
                            analysis.replaceUse(sym, valOpr);

                            // insert before
                            // load     %glb_val_<varName>, %glb_ptr_<varName>, 0
                            it = stmts.insert(it, { IntermediateRepresentation::LOAD, IntermediateRepresentation::i32, valOpr, ptrOpr, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, 0) } ) + 1;

                            // insert after
                            // store    %glb_val_<varName>, %glb_ptr_<varName>, 0
                            it = stmts.insert(it + 1, { IntermediateRepresentation::STORE, IntermediateRepresentation::i32, valOpr, ptrOpr, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, 0) } ) - 1;
                        }
                    } else if (analysis.use.count(sym)) {
                        if (sym.getIsPointer())
                            analysis.replaceUse(sym, ptrOpr);
                        else {
                            analysis.replaceUse(sym, valOpr);
                            // insert before
                            // load     %glb_val_<varName>, %glb_ptr_<varName>, 0
                            it = stmts.insert(it, { IntermediateRepresentation::LOAD, IntermediateRepresentation::i32, valOpr, ptrOpr, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, 0) } ) + 1;
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
                    // %dest
                    replaceList.push_back(replaceDest);
                    // func
                    replaceList.push_back(ops[1]);

                    std::vector<IntermediateRepresentation::IROperand> paramOpr;
                    for (int i = 1 + 1; i <= paramCount + 1; i++) {
                        // prepare parameters
                        // param    [%<funcName>_arg_%x, -(x-4)]
                        auto tmpOpr = IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, "");
                        if (ops[i].getIrOpType() == IntermediateRepresentation::Var)
                            tmpOpr.setVarName(funcName + "_arg_" + ops[i].getVarName());
                        else
                            tmpOpr.setVarName(funcName + "_arg_imm_" + std::to_string(i));
                        if (i >= 6) {
                            tmpOpr.setValue(-(i - 5));
                        } else {
                            tmpOpr.setValue(i - 2);
                        }
                        replaceList.push_back(tmpOpr);
                        paramOpr.push_back(tmpOpr);
                    }
                    // insert placeholders
                    if (paramCount < 4) {
                        for (int i = paramCount + 1; i <= 4; i++) {
                            auto tmpOpr = IntermediateRepresentation::IROperand(IntermediateRepresentation::t_void, "");
                            tmpOpr.setVarName(funcName + "_placeholder_" + std::to_string(i - 1));
                            tmpOpr.setValue(i - 1);
                            replaceList.push_back(tmpOpr);
                            // param        [ %<func>_arg_placeholder_x, null ]
                            paramOpr.push_back(tmpOpr);
                        }
                    }

                    for (auto& opr : paramOpr)
                        it = stmts.insert(it, { IntermediateRepresentation::PARAM, IntermediateRepresentation::i32,
                                                { opr } }) + 1;

                    if (paramCount > 4) {
                        for (int i = 6; i <= paramCount + 1; i++) {
                            // insert before
                            // stk_str      %replaceList[i], #paramOpr[i].getValue()
                            it = stmts.insert(it, {
                                IntermediateRepresentation::STK_STR, IntermediateRepresentation::i32, replaceList[i],
                                IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, paramOpr[i - 2].getValue())
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
                } else {
                    // return 0;
                    stmts.emplace_back(IntermediateRepresentation::RETURN, IntermediateRepresentation::i32, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, 0));
                }
            }

        }

    public:
        Translator() = default;
        ~Translator() override = default;
        explicit Translator(IntermediateRepresentation::IRProgram irProgram) : TranslatorBase(std::move(irProgram)) { }

        Instruction::InstructionStream doTranslation() override {
            InstructionStream ins, dataIns;
            // TODO
            auto functions = irProgram.getFunctions();
            std::unordered_map<std::string, std::string> globalMapping;
            std::unordered_set<IntermediateRepresentation::IROperand> globalSymbols;

            auto immNeedProc = [&] (int imm, int immLen) {
                // judge if imm is too big
                switch (immLen) {
                    case 8:
                        return imm < 0 || imm > 255;
                    case -8:
                        return imm < 0 || imm > 255;
                    case 10:
                        return imm < 0 || imm > 1024;
                    case 12:
                        return imm < 0 || imm > 4095;
                    case -12:
                        return imm > 4095 || imm < -4095;
                    case 16:
                        return imm < 0 || imm > 65535;
                    default:
                        throw std::runtime_error("Invalid immediate number length: " + std::to_string(immLen));
                }
            };

            auto loadImm = [&] (int imm) {
                ins << LoadInstruction(r9, imm);
                return r9;
            };

            ins << DotInstruction(Instruction::DotInstruction::TEXT, "");
            ins << DotInstruction(Instruction::DotInstruction::GLOBL, "main");

            // globalIns
            procGlobal(ins, dataIns, globalMapping, globalSymbols);

            for(auto& func : functions) {
                auto stackLayout = Util::StackScheme { };

                /* preprocess function
                 * rewrite  mod
                 * rewrite  div
                 * replace  globals
                 * allocate space
                 * */
                preProcFunc(func, stackLayout, globalMapping, globalSymbols);

                if (isDebug)
                    std::cout << "After preprocessing: " << std::endl << func.toString() << std::endl;

                allocator = std::make_unique<allocator_t>(allocator_t (&stackLayout, &func));

                auto allocation = allocator->getAllocation();
                auto variables = allocator->getVariables();
                auto totalColours = allocator->getTotalColours();
                const std::vector<IntermediateRepresentation::Statement>& stmts = func.getStatements();

                if (isDebug) {
                    std::cout << "Translator: Register assignment complete" << std::endl;
                    std::cout << "After register assignment: " << std::endl << func.toString() << std::endl;

                    std::cout << "Allocation scheme: " << std::endl;
                    for (auto& alloc : allocation)
                        std::cout << "\t" << alloc.first.toString() << ": " << numToReg[alloc.second] << std::endl;
                }

                // mapping colours
//                std::list<Operands::Register> remainRegisters;
//                std::unordered_map<size_t, Operands::Register> colourScheme;
                std::unordered_map<IntermediateRepresentation::IROperand, Operands::Register> mapping;
                Operands::RegisterList list;
                size_t pushSize = 0;

                list.emplaceRegister(r4, r5, r6, r7, r8, r9, r10);

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
                    int sub = (int) std::min(remainStackSize, (size_t) 1024);
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
                                if (ops[0].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                    int imm = ops[0].getValue();
                                    if (imm)
                                        ins << BranchInstruction(B, ops[1].getStrValue());
                                    else
                                        ins << BranchInstruction(B, ops[2].getStrValue());
                                } else {
                                    ins << ComparisonInstruction(CMP, mapping.at(ops[0]), Operands::Operand2(imm8(1)));
                                    auto breq = BranchInstruction(B, ops[1].getStrValue());
                                    breq.setCondition(Instruction::Condition::Cond_Equal);
                                    ins << std::move(breq);
                                    ins << BranchInstruction(B, ops[2].getStrValue());
                                }
                            }
                        }
                            break;
                        case IntermediateRepresentation::ADD: {
                            /*
                             * add i32 %dest, i32 %opr1, i32 %opr2
                             * */
//#warning "Imm12 not implemented"
                            auto dest = mapping.at(ops[0]), opr1 = mapping.at(ops[1]);
                            if (ops[2].getIrOpType() == IntermediateRepresentation::Var)
                                ins << AdditionInstruction(dest, opr1, mapping.at(ops[2]));
                            else {
                                int imm = ops[2].getValue();
                                if (immNeedProc(imm, 12))
                                    ins << AdditionInstruction(dest, opr1, Operands::Operand2(loadImm(imm)));
                                else
                                    ins << AdditionInstruction(dest, opr1, imm12(imm));
                            }
                        }
                            break;
                        case IntermediateRepresentation::MUL: {
//#warning "Imm12 not implemented"
                            auto dest = mapping.at(ops[0]), opr1 = mapping.at(ops[1]);
                            if (ops[2].getIrOpType() == IntermediateRepresentation::Var)
                                ins << MultiplicationInstruction(dest, opr1, mapping.at(ops[2]));
                            else {
                                int imm = ops[2].getValue();
                                if (!(imm & (imm - 1)) && imm >= 0) {
                                    // 2^n
                                    if (imm == 0)
                                        ins << MoveInstruction(dest, imm16(0));
                                    else {
                                        if (dest != opr1)
                                            ins << MoveInstruction(dest, opr1);
                                        ins << ShiftInstruction(Instruction::LSL, dest, dest, floor(log2(imm)));
                                    }
                                } else {
                                    ins << MultiplicationInstruction(dest, opr1, Operands::Operand2(loadImm(imm)));
                                }
                            }
                        }
                            break;
                        case IntermediateRepresentation::SUB: {
//#warning "Imm12 not implemented"
                            if (ops[1].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                auto dest = mapping.at(ops[0]), opr2 = mapping.at(ops[2]);
                                if (ops[2].getIrOpType() == IntermediateRepresentation::Var) {
                                    ins << SubtractionInstruction(dest, loadImm(ops[1].getValue()), opr2);
                                } else {
                                    int imm = ops[1].getValue() - ops[2].getValue();
                                    ins << LoadInstruction(dest, imm);
                                }
                            } else {
                                auto dest = mapping.at(ops[0]), opr1 = mapping.at(ops[1]);
                                if (ops[2].getIrOpType() == IntermediateRepresentation::Var)
                                    ins << SubtractionInstruction(dest, opr1, mapping.at(ops[2]));
                                else {
                                    int imm = ops[2].getValue();
                                    if (immNeedProc(imm, 12))
                                        ins << SubtractionInstruction(dest, opr1, Operands::Operand2(loadImm(imm)));
                                    else
                                        ins << SubtractionInstruction(dest, opr1, imm12(imm));
                                }
                            }
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
//#warning "Imm not implemented"
                                        int imm = 4 * pushSize + (pos - 5) * 4;
                                        if (immNeedProc(imm, -12)) {
                                            ins << LoadInstruction(mapping.at(opr), Operands::LoadSaveOperand(fp, loadImm(imm), true));
                                        } else
                                            ins << LoadInstruction(mapping.at(opr), Operands::LoadSaveOperand(fp, imm, true));
                                    }
                                } else {
                                    /*
                                     * When pos is negative, it means PARAM is for caller
                                     * param    %param, -1
                                     *
                                     * str      %param, [sp, #((-pos - 1) * 4)]
                                     * */
//#warning "Imm not implemented"

                                    int imm = (-pos - 1) * 4;
                                    if (immNeedProc(imm, -12)) {
                                        ins << SaveInstruction(mapping.at(opr), Operands::LoadSaveOperand(sp, loadImm(imm), true));
                                    } else
                                    ins << SaveInstruction(mapping.at(opr), Operands::LoadSaveOperand(sp, imm, true));
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
                                if (ops[0].getIrOpType() == IntermediateRepresentation::Var) {
                                    if (mapping.at(ops[0]) != r0)
                                        ins << MoveInstruction(r0, mapping.at(ops[0]));
                                } else {
//#warning "Imm16 is not implemented"
                                    int imm = ops[0].getValue();
                                    if (immNeedProc(imm, 16))
                                        ins << MoveInstruction(r0, Operands::Operand2(loadImm(imm)));
                                    else
                                        ins << MoveInstruction(r0, imm16(imm));
//                                    ins << MoveInstruction(r0, imm16(ops[0].getValue()));
                                }
                            }

                            // Epilogue
                            // add      sp, sp #stack_size
                            for (size_t remainStackSize = stackSize; remainStackSize; ) {
                                int sub = (int) std::min(remainStackSize, (size_t) 1024);
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
                                // ldr      %dest, =<value>
                                // or
                                // mov      %dest, #<value>
                                int imm = ops[1].getValue();
                                if (immNeedProc(imm, 10))
                                    ins << LoadInstruction(mapping.at(ops[0]), imm);
                                else
                                    ins << MoveInstruction(mapping.at(ops[0]), imm16(imm));
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
//#warning "Imm16 not implemented"
                            int imm = stackSize - ops[1].getValue();
                            if (immNeedProc(imm, 16))
                                ins << MoveInstruction(mapping.at(ops[0]), loadImm(imm));
                            else
                                ins << MoveInstruction(mapping.at(ops[0]), imm16(imm));

//                            ins << MoveInstruction(mapping.at(ops[0]), imm16(stackSize - ops[1].getValue()));
                            ins << AdditionInstruction(mapping.at(ops[0]), mapping.at(ops[0]), sp);
                        }
                            break;
                        case IntermediateRepresentation::STK_LOAD: {
                            /*
                             * stk_load     i32 %dest, %off
                             *
                             * ldr          %dest, [fp, #off]
                             * */
//#warning "Imm not implemented"
                            if (ops[1].getIrOpType() == IntermediateRepresentation::Var) {
                                ins << LoadInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(fp, mapping.at(ops[1]), true));
                            } else {
                                auto stk_pointer = fp;
                                int imm = ops[1].getValue();
                                if (imm < 0) {
                                    imm = -(4 * pushSize - (4 + imm) * 4);
                                }
                                imm = -imm;
                                if (immNeedProc(imm, -12))
                                    ins << LoadInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(stk_pointer, loadImm(imm), true));
                                else
                                    ins << LoadInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(stk_pointer, imm,true));
                            }
                        }
                            break;
                        case IntermediateRepresentation::STK_STR: {
                            /*
                             * stk_str      i32 %src, %off
                             *
                             * str          %src, [fp, #off]
                             * */
//#warning "Imm not implemented"

                            if (ops[1].getIrOpType() == IntermediateRepresentation::Var) {
                                ins << SaveInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(fp, mapping.at(ops[1]),true));
                            } else {
                                int imm = ops[1].getValue();
                                auto stk_pointer = fp;
                                if (ops.size() > 2) {
                                    // caller str
                                    stk_pointer = sp;
                                    imm = -4 * imm;
                                }
                                imm = -imm;
                                if (immNeedProc(imm, -12))
                                    ins << SaveInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(stk_pointer, loadImm(imm),true));
                                else
                                    ins << SaveInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(stk_pointer, imm,true));
                            }
                        }
                            break;
                        case IntermediateRepresentation::LOAD: {
                            /*
                             * load         i32 %dest, *i32 %base, i32 %off
                             *
                             * ldr          %dest, [%base, #off]
                             * */
//#warning "Imm not implemented"

                            if (ops[2].getIrOpType() == IntermediateRepresentation::Var) {
                                ins << LoadInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(mapping.at(ops[1]), mapping.at(ops[2]), true));
                            } else {
                                int imm = ops[2].getValue();
                                if (immNeedProc(imm, -12))
                                    ins << LoadInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(mapping.at(ops[1]), loadImm(imm), true));
                                else
                                    ins << LoadInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(mapping.at(ops[1]), imm, true));

                            }
                        }
                            break;
                        case IntermediateRepresentation::STORE: {
                            /*
                             * store        i32 %source, *i32 %base, i32 %off
                             *
                             * str          %source, [%base, #off]
                             * */
//#warning "Imm not implemented"
                            if (ops[0].getIrOpType() == IntermediateRepresentation::Var) {
                                if (ops[2].getIrOpType() == IntermediateRepresentation::Var) {
                                    ins << SaveInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(mapping.at(ops[1]), mapping.at(ops[2]), true));
                                } else {
                                    int imm = ops[2].getValue();
                                    if (immNeedProc(imm, -12))
                                        ins << SaveInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(mapping.at(ops[1]), loadImm(imm), true));
                                    else
                                        ins << SaveInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(mapping.at(ops[1]), imm, true));
                                }
                            } else {
                                if (ops[2].getIrOpType() == IntermediateRepresentation::Var) {
                                    ins << SaveInstruction(r10, Operands::LoadSaveOperand(mapping.at(ops[1]), mapping.at(ops[2]), true));
                                } else {
                                    int immOff = ops[2].getValue();
                                    ins << LoadInstruction(r10, ops[0].getValue());
                                    if (immNeedProc(immOff, -12))
                                        ins << SaveInstruction(r10, Operands::LoadSaveOperand(mapping.at(ops[1]), loadImm(immOff), true));
                                    else
                                        ins << SaveInstruction(r10, Operands::LoadSaveOperand(mapping.at(ops[1]), immOff, true));
                                }
                            }
                        }
                            break;
                        case IntermediateRepresentation::CMP_EQ: {
                            /*
                             * cmp_xx       %dest, %opr1, %opr2
                             *
                             * cmp          %opr1, %opr2
                             * moveq        %dest, #1
                             * */
//#warning "Imm8 is not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                int imm = ops[2].getValue();
                                if (immNeedProc(imm, -8))
                                    ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(loadImm(imm)));
                                else
                                    ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(imm)));
//                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(ops[2].getValue())));
                            } else {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(mapping.at(ops[2])));
                            }
//#warning "Imm16 is not implemented"
                            ins << MoveInstruction(mapping.at(ops[0]), imm16(0));
                            auto moveq = MoveInstruction(mapping.at(ops[0]), imm16(1));
                            moveq.setCondition(Instruction::Condition::Cond_Equal);
                            ins << std::move(moveq);
                        }
                            break;
                        case IntermediateRepresentation::CMP_NE: {
//#warning "Imm8 is not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                int imm = ops[2].getValue();
                                if (immNeedProc(imm, -8))
                                    ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(loadImm(imm)));
                                else
                                    ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(imm)));
//                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(ops[2].getValue())));
                            } else {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(mapping.at(ops[2])));
                            }
//#warning "Imm16 is not implemented"
                            ins << MoveInstruction(mapping.at(ops[0]), imm16(0));
                            auto moveq = MoveInstruction(mapping.at(ops[0]), imm16(1));
                            moveq.setCondition(Instruction::Condition::Cond_NotEqual);
                            ins << std::move(moveq);
                        }
                            break;
                        case IntermediateRepresentation::CMP_SGE: {
//#warning "Imm8 is not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                int imm = ops[2].getValue();
                                if (immNeedProc(imm, -8))
                                    ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(loadImm(imm)));
                                else
                                    ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(imm)));
//                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(ops[2].getValue())));
                            } else {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(mapping.at(ops[2])));
                            }
//#warning "Imm16 is not implemented"
                            ins << MoveInstruction(mapping.at(ops[0]), imm16(0));
                            auto moveq = MoveInstruction(mapping.at(ops[0]), imm16(1));
                            moveq.setCondition(Instruction::Condition::Cond_SGreaterEqual);
                            ins << std::move(moveq);
                        }
                            break;
                        case IntermediateRepresentation::CMP_SLE: {
//#warning "Imm8 is not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                int imm = ops[2].getValue();
                                if (immNeedProc(imm, -8))
                                    ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(loadImm(imm)));
                                else
                                    ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(imm)));
//                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(ops[2].getValue())));
                            } else {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(mapping.at(ops[2])));
                            }
//#warning "Imm16 is not implemented"
                            ins << MoveInstruction(mapping.at(ops[0]), imm16(0));
                            auto moveq = MoveInstruction(mapping.at(ops[0]), imm16(1));
                            moveq.setCondition(Instruction::Condition::Cond_SLessEqual);
                            ins << std::move(moveq);
                        }
                            break;
                        case IntermediateRepresentation::CMP_SGT: {
//#warning "Imm8 is not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                int imm = ops[2].getValue();
                                if (immNeedProc(imm, -8))
                                    ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(loadImm(imm)));
                                else
                                    ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(imm)));
//                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(ops[2].getValue())));
                            } else {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(mapping.at(ops[2])));
                            }
//#warning "Imm16 is not implemented"
                            ins << MoveInstruction(mapping.at(ops[0]), imm16(0));
                            auto moveq = MoveInstruction(mapping.at(ops[0]), imm16(1));
                            moveq.setCondition(Instruction::Condition::Cond_SGreater);
                            ins << std::move(moveq);
                        }
                            break;
                        case IntermediateRepresentation::CMP_SLT: {
//#warning "Imm8 is not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                int imm = ops[2].getValue();
                                if (immNeedProc(imm, -8))
                                    ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(loadImm(imm)));
                                else
                                    ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(imm)));
//                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(imm8(ops[2].getValue())));
                            } else {
                                ins << ComparisonInstruction(CMP, mapping.at(ops[1]), Operands::Operand2(mapping.at(ops[2])));
                            }
//#warning "Imm16 is not implemented"
                            ins << MoveInstruction(mapping.at(ops[0]), imm16(0));
                            auto moveq = MoveInstruction(mapping.at(ops[0]), imm16(1));
                            moveq.setCondition(Instruction::Condition::Cond_SLess);
                            ins << std::move(moveq);
                        }
                            break;
                        case IntermediateRepresentation::GLB_ARR: {
                            ins << LoadInstruction(mapping.at(ops[0]), globalMapping[ops[1].getStrValue()]);
                        }
                            break;
                        case IntermediateRepresentation::GLB_CONST:
                        case IntermediateRepresentation::GLB_VAR: {
                            /*
                             * Global variables have modified
                             *
                             * glb_xxx      %var_pos, "<global name>"
                             * */
//                            ins << LoadInstruction(mapping.at(ops[0]), "=" + globalPtrToVal[globalMapping[ops[1].getStrValue()]]);
                             ins << LoadInstruction(mapping.at(ops[0]), globalMapping[ops[1].getStrValue()]);
                        }
                            break;
                        case IntermediateRepresentation::LSH: {
                            /*
                             * lsh      %dest, %src, %num
                             *
                             * lsl      %dest, %src, %num
                             * */
//#warning "Imm not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                int imm = ops[2].getValue();
                                if (imm == 0) {
                                    if (mapping.at(ops[0]) != mapping.at(ops[1]))
                                        ins << MoveInstruction(mapping.at(ops[0]), mapping.at(ops[1]));
                                    break;
                                }
                                if (immNeedProc(imm, 8))
                                    ins << ShiftInstruction(LSL, mapping.at(ops[0]), mapping.at(ops[1]), loadImm(imm));
                                else
                                    ins << ShiftInstruction(LSL, mapping.at(ops[0]), mapping.at(ops[1]), imm);
//                                ins << ShiftInstruction(LSL, mapping.at(ops[0]), mapping.at(ops[1]), ops[2].getValue());
                            } else
                                ins << ShiftInstruction(LSL, mapping.at(ops[0]), mapping.at(ops[1]), mapping.at(ops[2]));
                        }
                            break;
                        case IntermediateRepresentation::RSH: {
//#warning "Imm not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                int imm = ops[2].getValue();
                                if (imm == 0) {
                                    if (mapping.at(ops[0]) != mapping.at(ops[1]))
                                        ins << MoveInstruction(mapping.at(ops[0]), mapping.at(ops[1]));
                                    break;
                                }
                                if (immNeedProc(imm, 8))
                                    ins << ShiftInstruction(LSR, mapping.at(ops[0]), mapping.at(ops[1]), loadImm(imm));
                                else
                                    ins << ShiftInstruction(LSR, mapping.at(ops[0]), mapping.at(ops[1]), imm);
//                                ins << ShiftInstruction(LSR, mapping.at(ops[0]), mapping.at(ops[1]), ops[2].getValue());
                            } else
                                ins << ShiftInstruction(LSR, mapping.at(ops[0]), mapping.at(ops[1]), mapping.at(ops[2]));
                        }
                            break;
                        case IntermediateRepresentation::OR: {
                            /*
                             * or       %dest, %opr1, %opr2
                             *
                             * orr      %dest, %opr1, %opr2
                             * */
//#warning "Imm8 not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                int imm = ops[2].getValue();
                                if (immNeedProc(imm, 8))
                                    ins << BitInstruction(ORR, mapping.at(ops[0]), mapping.at(ops[1]), loadImm(imm));
                                else
                                    ins << BitInstruction(ORR, mapping.at(ops[0]), mapping.at(ops[1]), imm8(imm));
//                                ins << BitInstruction(ORR, mapping.at(ops[0]), mapping.at(ops[1]), imm8(ops[2].getValue()));
                            } else {
                                ins << BitInstruction(ORR, mapping.at(ops[0]), mapping.at(ops[1]), mapping.at(ops[2]));
                            }
                        }
                            break;
                        case IntermediateRepresentation::AND: {
//#warning "Imm8 not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                int imm = ops[2].getValue();
                                if (immNeedProc(imm, 8))
                                    ins << BitInstruction(AND, mapping.at(ops[0]), mapping.at(ops[1]), loadImm(imm));
                                else
                                    ins << BitInstruction(AND, mapping.at(ops[0]), mapping.at(ops[1]), imm8(imm));
//                                ins << BitInstruction(AND, mapping.at(ops[0]), mapping.at(ops[1]), imm8(ops[2].getValue()));
                            } else {
                                ins << BitInstruction(AND, mapping.at(ops[0]), mapping.at(ops[1]), mapping.at(ops[2]));
                            }
                        }
                            break;
                        case IntermediateRepresentation::XOR: {
//#warning "Imm8 not implemented"
                            if (ops[2].getIrOpType() == IntermediateRepresentation::ImmVal) {
                                int imm = ops[2].getValue();
                                if (immNeedProc(imm, 8))
                                    ins << BitInstruction(EOR, mapping.at(ops[0]), mapping.at(ops[1]), loadImm(imm));
                                else
                                    ins << BitInstruction(EOR, mapping.at(ops[0]), mapping.at(ops[1]), imm8(imm));
//                                ins << BitInstruction(EOR, mapping.at(ops[0]), mapping.at(ops[1]), imm8(ops[2].getValue()));
                            } else {
                                ins << BitInstruction(EOR, mapping.at(ops[0]), mapping.at(ops[1]), mapping.at(ops[2]));
                            }
                        }
                            break;
                        case IntermediateRepresentation::NOT: {
                            /*
                             * not      %dest
                             * cmp      %dest, 0
                             * moveq    %dest, 1
                             * movne    %dest, 0
                             * */
                            auto dest = mapping.at(ops[0]);
                            ins << ComparisonInstruction(CMP, dest, imm8(0));
                            auto moveq = MoveInverseInstruction(dest, imm8(1)), movne = MoveInverseInstruction(dest, imm8(0));
                            moveq.setCondition(Instruction::Condition::Cond_Equal);
                            movne.setCondition(Instruction::Condition::Cond_NotEqual);
                            ins << std::move(moveq) << std::move(movne);
                        }
                            break;
                        case IntermediateRepresentation::PHI:
                            throw std::runtime_error("Invalid IR: Phi function in SSA should not appear in backend IR. Entailed IR: " + stmt.toString());
                    }
                }

                // TODO Imm Fix
            }

            // insert data segment
            if (!dataIns.empty()) {
                ins << DotInstruction(Instruction::DotInstruction::DATA, "");
                ins.insert(ins.end(), dataIns.begin(),  dataIns.end());
                ins << DotInstruction(Instruction::DotInstruction::END, "");
            }
            return ins;
        }
    };
}

#endif //SYSYBACKEND_IRTRANSLATOR_H
