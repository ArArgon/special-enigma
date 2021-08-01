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

    class [[deprecated]] SimpleTranslator : public TranslatorBase {
    private:
        /**
         * Label/heap description:
         *
         *  1. Temporary labels:
         *      -> store i32 0x222ffff, i32 * %23;
         *
         *      .LB_<funcName>_tmp_1:
         *              .long   16773666    @ 0x222ffff
         *      ...
         *              ldr     targetReg, .LB_FUNC_1
         *
         *  2. Static variables:
         *      -> (glb_var i32), %dest, i32 255161
         *      .LB_GLB_2:
         *              .long   dest
         *      dest:
         *              .long   255161
         *      ...
         *              ldr     tmpReg, .LB_GLB_2
         *              ldr     targetReg, [tmpReg]
         *              add     targetReg, targetReg, #3
         *              str     targetReg, [tmpReg]
         *
         *   3. Branches:
         *      ->  4:
         *      ->  %2 = load i32, i32* %1
         *      ->  br label %4
         *
         *      .LB_<funcName>_4:
         *              ldr     r2, r1
         *              b       .LB_<funcName>_4
         *
         *   4. Strings:
         *      ## printf("Hello world!");
         *      -> glb_const *i32, %str1, "Hello world!\0"
         *      -> call i32, "printf", %1, %str.1
         *
         *      .LB_<funcName>_5:
         *              .long   .LB_STR_1
         *      .LB_STR_1:
         *              .asciz  "Hello world!"
         *              ...
         *      other:
         *              ldr     r0, .LB_<funcName>_5
         *              bl      printf
         *
         *   Implementations:
         *      a.  label_map: records the mapping between original branch name and new branch name
         *          eg1: label_map["4"] = ".LB_<funcName>_4";   // procedural labels
         *          eg2: label_map["dest"] = "dest";            // static variables
         *          eg3: label_map["str.1"] = "str.1";          // strings
         *          eg3: LB_FUNC_1 <N/A>                        // this map doesn't store the local labels
         *
         *      b.  relative_map: records the relative addresses of static labels
         *          eg1: relative_map["str.1"] = "LB_<funcName>_5";     // strings
         *          eg2: relative_map["dest"] = "LB_GLB_2";             // static variables
         *
         *      c.  label_type: the type of labels
         *          eg1: label_type['4"] = 0;                   // common label
         *          eg2: label_type["LB_<funcName>_tmp_1"] = 1; // temporary or direct label
         *          eg3: label_type["dest"] = 2;                // static or relative label
         *          eg4: label_type["LB_GLB_2"] = 1;            // temporary or direct label
         *          eg5: label_type["LB_STR_1"] = 1;            // temporary or direct label
         * */

        std::unordered_map<std::string, int> label_type;
        std::unordered_map<std::string, std::string> label_map,     // mapping labels to their modified name
                                                     relative_map;  // mapping static vars to their relative label
        Instruction::InstructionStream global;

        void appendAns(const InstructionStream& instructionStream) {
            ans.insert(ans.end(), instructionStream.begin(), instructionStream.end());
        };

        void loadHeap() {
            /*
             * -> glb_const str %str, "This is a text";
             * @ For global constant, save it as a direct label:
             * LB_GBL_str.0:
             *          .asciz      "This is a text"
             *
             *
             * -> glb_var i32 %maxn, 0x333fff;
             * @ For global constant, save it as relative labels:
             * LB_GBL_2:
             *          .long       maxn
             * maxn:
             *          .word       3358719     @ 0x333fff
             * */

            /**
             * Remember to maintain the maps!!
             *      - label_type,
             *      - label_map,
             *      - relative_map
             */

            const auto&& lb_pfx = std::string(".LB_GLB_");
            int str_cnt = 0, cnt = 0;

            auto& globals = irProgram.getGlobal();
            for (auto& glb : globals) {
                auto& ops = glb.getOps();
                switch (glb.getStmtType()) {
                    case IntermediateRepresentation::GLB_CONST: {
                        if (ops.size() != 2)
                            throw std::runtime_error("Invalid IR: invalid arguments for GLB_CONST" );
                        /*
                         * ops[0]: variable
                         * ops[1]: value
                         */
                        if (ops[0].getIrOpType() != IntermediateRepresentation::Var)
                            throw std::runtime_error("Invalid IR: invalid argument type for GLB_CONST");
                        auto& varName = ops[0].getVarName();
                        if (varName.empty())
                            throw std::runtime_error("Invalid IR: name of a global constant should not be empty");
                        auto dataType = ops[1].getIrDataType();
                        if (dataType == IntermediateRepresentation::t_void)
                            throw std::runtime_error("Invalid IR: invalid argument data type for GLB_CONST");
                        if (dataType == IntermediateRepresentation::str) {
                            std::string lb_rel = lb_pfx + "_str." + std::to_string(++str_cnt);
                            /*
                             * .LB_GLB_str.<i>:
                             *      .long   <varName>
                             * <varName>:
                             *      .asciz  <var>
                             * */
                            global << Instruction::LabelInstruction(lb_rel)
                                   << Instruction::DotInstruction(Instruction::DotInstruction::LONG, varName)
                                   << Instruction::LabelInstruction(varName)
                                   << Instruction::DotInstruction(Instruction::DotInstruction::ASCIZ, ops[1].getStrValue());
                            // register labels:
                            label_type[lb_rel] = 2;     // relative
                            label_type[varName] = 1;    // direct

                            relative_map[varName] = lb_rel;
                            label_map[varName] = varName;
                        } else {
                            // integer value
                            /*
                             * <varName>:
                             *      .long   <var>
                             * */
                            global << Instruction::LabelInstruction(varName)
                                   << Instruction::DotInstruction(Instruction::DotInstruction::LONG, ops[1].getValue());
                            label_type[varName] = 1;    // direct
                            label_map[varName] = varName;
                        }

                    }
                        break;
                    case IntermediateRepresentation::GLB_VAR: {
                        if (ops.size() > 2 || ops.empty())
                            throw std::runtime_error("Invalid IR: invalid arguments for GLB_VAR");
                        if (ops[0].getIrOpType() == IntermediateRepresentation::Null || ops[0].getIrDataType() == IntermediateRepresentation::t_void)
                            throw std::runtime_error("Invalid IR: destination variable for GLB_VAR is invalid");
                        if (ops.size() == 1 && ops[0].getIrDataType() == IntermediateRepresentation::str)
                            throw std::runtime_error("Invalid IR: invalid arguments for GLB_VAR");
                        if (ops[0].getIrOpType() != IntermediateRepresentation::Var)
                            throw std::runtime_error("Invalid IR: invalid types of arguments for GLB_VAR");
                        auto& varName = ops[0].getVarName();
                        if (varName.empty())
                            throw std::runtime_error("Invalid IR: name of a global variable should not be empty");

                        std::string lb_rel = lb_pfx +
                                ((ops[0].getIrDataType() == IntermediateRepresentation::str)
                                    ? ("_str." + std::to_string(++str_cnt))
                                    : ("." + std::to_string(++cnt)));

                        /*
                         * <lb_rel>:
                         *      .long   <varName>
                         * <varName>:
                         *      .<type> <var>
                         * */

                        global << Instruction::LabelInstruction(lb_rel)
                               << Instruction::DotInstruction(Instruction::DotInstruction::LONG, varName)
                               << Instruction::LabelInstruction(varName);

                        if (ops[0].getIrDataType() == IntermediateRepresentation::str)
                            global << Instruction::DotInstruction(Instruction::DotInstruction::ASCIZ, ops[1].getStrValue());
                        else
                            global << Instruction::DotInstruction(Instruction::DotInstruction::LONG, ops[1].getValue());

                        // register labels:
                        label_type[lb_rel] = 2;     // relative
                        label_type[varName] = 1;    // direct

                        relative_map[varName] = lb_rel;
                        label_map[varName] = varName;
                    }
                        break;
                    default:
                        throw std::runtime_error("Invalid IR: Unexpected statements in global declarations: " + std::to_string(glb.getStmtType()));
                }
            }
        }

        void processFunction(const IntermediateRepresentation::Function & function) {
            // stack layout
            int stack_size = 0, fp_off = 0, sp_off = 0;
            std::unordered_map<std::string, size_t> stk_mapping; // relative to the beginning of the stack
            std::set<std::string> setAlloca;
            auto& funcName = function.getFunName();
            auto& statements = function.getStatements();
            auto& params = function.getParameters();
            std::string&& lb_pfx = ".LB_" + funcName + "_";
            int cnt = 0;

            auto new_on_stk = [&] (const std::string& var_name) {
                int pos = -(stack_size += 4);
                stk_mapping[var_name] = pos;
                return pos;
            };

            Instruction::InstructionStream front_wrapper, end_wrapper, body;

            auto load_to_reg = [&] (const Operands::Register& targetReg, const std::string& variable) {
                if (stk_mapping.count(variable)) {
                    // ldr      targetReg, [fp, #offset]
                    auto off = stk_mapping[variable];
                    body << Instruction::LoadInstruction(targetReg, Operands::LoadSaveOperand(fp, off, true));
                } else if (label_type.count(variable)) {
                    switch (label_type[variable]) {
                        case 2: // relative
                            /*
                             * ldr      targetReg, rel[variable]
                             * ldr      targetReg, [targetReg]
                             * */
                            body << Instruction::LoadInstruction(targetReg, relative_map[variable])
                                 << Instruction::LoadInstruction(targetReg, Operands::LoadSaveOperand(targetReg));
                            break;
                        case 1: // direct
                            /*
                             * ldr      targetReg, variable
                             * */
                            body << Instruction::LoadInstruction(targetReg, variable);
                            break;
                        default:
                            throw std::runtime_error("Unsupported label type: " + std::to_string(label_type[variable]));
                    }
                    std::string srcLabel = label_map[variable];
                    body << Instruction::LoadInstruction(targetReg, srcLabel);
                } else {
                    // error: no such variable
                    throw std::runtime_error("No such variable: " + variable);
                }
            };

            auto push_reg = [&] (const Operands::Register& reg) {
                /*
                 * push     { <reg> }
                 * */
                body << Instruction::PushInstruction(Operands::RegisterList(reg.getReg()));
            };

            auto pop_reg = [&] (const Operands::Register& reg) {
                /*
                 * pop     { <reg> }
                 * */
                body << Instruction::PopInstruction(Operands::RegisterList(reg.getReg()));
            };

            auto tmp_reg = [&] (const Operands::Register& cur) {
                for (int i = R0; i <= R10; i++)
                    if (cur.getReg() != i)
                        return Operands::Register(static_cast<ARMv7_Register>(i));
                return r10;
            };

            auto save_to_stk = [&] (const Operands::Register& source, const std::string& variable) {
                if (stk_mapping.count(variable)) {
                    // str      targetReg, [fp, #offset]
                    auto off = stk_mapping[variable];
                    body << Instruction::SaveInstruction(source, Operands::LoadSaveOperand(fp, off, true));
                } else if (label_type.count(variable)) {
                    if (label_type[variable] == 2) {
                        /*
                         * push     { tmp }
                         * ldr      tmp, rel[variable]
                         * str      source, [tmp]
                         * pop      { tmp }
                         * */
                        auto&& tmp = tmp_reg(source);
                        push_reg(tmp);
                        body << Instruction::LoadInstruction(tmp, relative_map[variable])
                             << Instruction::SaveInstruction(source, Operands::LoadSaveOperand(tmp));
                        pop_reg(tmp);
                    } else {
                        // disabled
                        if (enable_direct_label_write && label_type[variable] == 1) {
                            /*
                             * push     { tmp }
                             * ldr      tmp, =variable
                             * str      source, [tmp]
                             * pop      { tmp }
                             * */
                            auto&& tmp = tmp_reg(source);
                            push_reg(tmp);
                            body << Instruction::LoadInstruction(tmp, "=" + variable)
                                 << Instruction::SaveInstruction(source, Operands::LoadSaveOperand(tmp));
                            pop_reg(tmp);
                            return;
                        }
                        throw std::runtime_error("Unsupported label type: " + std::to_string(label_type[variable]));
                    }
                } else {
                    // error: no such variable
                    throw std::runtime_error("No such variable: " + variable);
                }
            };

            auto prepare_dest_var = [&] (const IntermediateRepresentation::IROperand& var) {
                // This is for the destination variable!
                if (var.getIrOpType() != IntermediateRepresentation::Var)
                    throw std::runtime_error("Invalid IR: not an SSA variable");
                auto& varName = var.getVarName();
                if (stk_mapping.count(varName) && !setAlloca.count(varName)) {
                    // This is not an SSA!
                    throw std::runtime_error("Invalid IR: SSA variable '" + varName + "' in function '" + funcName + "' should only be assigned once");
                }
                if (setAlloca.count(varName))
                    return 0;
                return new_on_stk(varName);
            };

            auto create_cmp_instruction = [&] (const auto& source1, const auto& source2) {
                switch (source1.getIrOpType()) {
                case IntermediateRepresentation::ImmVal:
                    throw std::runtime_error("Invalid IR: cmp source1 must be a variable");
                case IntermediateRepresentation::Var:
                    load_to_reg(r0, source1.getVarName());
                    break;
                case IntermediateRepresentation::Null:
                    throw std::runtime_error("Invalid IR: unexpected arguments for ICMP statement");
                }
                switch (source2.getIrOpType()) {
                    case IntermediateRepresentation::ImmVal: {
                        int32_t imm = source2.getValue();
                        // TODO
#warning "The support for Imm8m is not implemented."
                        body << Instruction::ComparisonInstruction(Instruction::CMP, r0, Operands::Operand2(imm8(imm)));
                    }
                        break;
                    case IntermediateRepresentation::Var: {
                        load_to_reg(r1, source2.getVarName());
                        body << Instruction::ComparisonInstruction(Instruction::CMP, r0, r1);
                    }
                        break;
                    case IntermediateRepresentation::Null: {
                        throw std::runtime_error("Invalid IR: unexpected arguments for ICMP        statement");
                    }
                }
            };

            /*
             * func_name:
             *      push    { fp, lr }
             *      mov     fp, sp
             * */
            front_wrapper << Instruction::LabelInstruction(funcName)
                          << Instruction::PushInstruction(Operands::RegisterList(FP, LR))
                          << Instruction::MoveInstruction(fp, Operands::Operand2(sp));

            //  LB_<funcName>_ret:
            end_wrapper << Instruction::LabelInstruction( lb_pfx + "_ret");
            label_type[funcName] = 0;           // common label
            label_type[lb_pfx + "_ret"] = 0;    // common label
            label_map[funcName] = funcName;

            // loading arguments into stacks
            int arg_len = params.size(), inReg = std::max(params.size(), (size_t) 4), pos;
            switch (inReg) {
                case 4:
                    pos = new_on_stk(params[3].getVarName());
                    // str      r3, [fp, #-(inReg-3)*4]
                    front_wrapper << SaveInstruction(r3, Operands::LoadSaveOperand(fp, pos, true));
                case 3:
                    pos = new_on_stk(params[2].getVarName());
                    // str      r2, [fp, #-(inReg-2)*4]
                    front_wrapper << SaveInstruction(r2, Operands::LoadSaveOperand(fp, pos, true));
                case 2:
                    pos = new_on_stk(params[1].getVarName());
                    // str      r1, [fp, #-(inReg-1)*4]
                    front_wrapper << SaveInstruction(r1, Operands::LoadSaveOperand(fp, pos, true));
                case 1:
                    pos = new_on_stk(params[0].getVarName());
                    // str      r0, [fp, #-inReg*4]
                    front_wrapper << SaveInstruction(r0, Operands::LoadSaveOperand(fp, pos, true));
                default:
                    break;
            }
            for (size_t i = arg_len - 1; i > 3; i++) {
                // arguments in the stack
                /*
                 * for function(a0, a1, a2, a3, ..., an):
                 *      a0, a1, a2, a3: stored in r0, r1, r2, r3
                 *      aN, a(N-1), ..., a4 are stored in the stack
                 * */
                stk_mapping[params[i].getVarName()] = 4 * (arg_len - 4);
            }

            // function body
            Instruction::Condition::Cond cmpFlag = Instruction::Condition::Cond_NO;
            for (auto& stmt : statements) {
                auto& ops = stmt.getOps();
                auto data_type = stmt.getDataType();
                switch (stmt.getStmtType()) {
                    case IntermediateRepresentation::BR: {
                        /*
                         * br   label %target
                         *
                         * br   i32 %cond, label %a, label %b
                         */
                        if (stmt.getDataType() == IntermediateRepresentation::label) {
                            // br   label %target
                            if (ops[0].getIrDataType() != IntermediateRepresentation::label)
                                throw std::runtime_error("Invalid branching IR: Arguments type invalid");
                            // b        <%a>
                            body << Instruction::BranchInstruction(B, label_map[ops[0].getVarName()]);
                        } else {
                            if (cmpFlag == Instruction::Condition::Cond_NO) // no comparison
                                throw std::runtime_error("Invalid branching IR: No comparison before conditional branching.");
                            auto&& br = Instruction::BranchInstruction(B, label_map[ops[0].getVarName()]);
                            br.setCondition(cmpFlag);
                            body << std::move(br);
                        }
                    }
                        break;
                    case IntermediateRepresentation::ADD: {
                        /**
                         * This is an example of generating arm ASM code using this framework.
                         *
                         * We assume that Ops[] of stmt has 3 operands: %dest, %src1, %src2.
                         * In this context, %dest & %src1 have to be variables, but %src2 could be either an imm<12> or a variable;
                         *
                         * The gists of this sub-programme are:
                         *      1. We examine the %dest and allocate space for it if it's not an alloca-variable;
                         *      2. We put the value of %src1 to r0;
                         *      3. We put the value of %src2 to r1. However, it's becoming a little bit tricky when %src2 is an immediate number.
                         *
                         * Things to pay attention to:
                         *      1. %dest must be an alloca-variable or completely new
                         *          * Using prepare_dest_var() to check %dest;
                         *            If %dest is new, then it will allocate stack space for %dest and return offset to fp.
                         *            If %dest is an alloca-var, it will return 0.
                         *            It will throw an exception when %dest is a used SSA variable.
                         *      2. Loading %src1 to r0:
                         *          load_to_reg(r0, "src1");   // provided func
                         *      3. %src2 could be an immediate number or a variable.
                         *          We denote %src as one of them.
                         *          * %src2 is an immediate number:
                         *              - Using Utilities::isTooLong() to check if %src is too big to be an immediate value in ASM.
                         *                  - If it's too long, then store it in a label and load it to a register temp_reg.
                         *                      * It's crucial to
                         *                  - If it's ok, then generate instructions:
                         *                      add     r0, r0, #<src2>
                         *                      save_to_stk(r0, "dest");    // provided func
                         *       4. If %src2 is a variable, we have instructions:
                         *          load_to_reg(r1, "src2");    // provided func
                         *          add     r0, r0, r1;
                         *          save_to_stk(r0, "dest");    // provided func
                         *
                         * And now, it's your time:
                         * TODO
                         * */
                    }
                        break;
                    case IntermediateRepresentation::MUL: {

                    }
                        break;
                    case IntermediateRepresentation::DIV: {

                    }
                        break;
                    case IntermediateRepresentation::MOD: {

                    }
                        break;
                    case IntermediateRepresentation::SUB: {

                    }
                        break;
                    case IntermediateRepresentation::CALL: {
                        /*
                         * call t_void, func, %ret, %par1, %par2, ...
                         * */
                        int arg_cnt = ops.size() - 2, tmp_stk_size = stack_size;
                        if (arg_cnt < 0)
                            throw std::runtime_error("Invalid call intermediate representation: too few arguments");
                        auto &func = ops[0], &ret = ops[1];

                        if (func.getIrDataType() != stmt.getDataType()) {
                            // return type unmatched
                            throw std::runtime_error("Invalid IR: unmatched return type");
                        }

                        /*
                         * %par1    r0
                         * %par2    r1
                         * %par3    r2
                         * %par4    r3
                         *
                         * %parN    stk
                         * %parN-1  stk
                         * ...
                         * %par5    stk
                         *
                         * */
                        for (int i = arg_cnt; i >= 5; i--) {
                            // push onto stacks
                            auto& par = ops[i + 1];
                            /*
                             * mov      r2, fp // tmp stack pointer
                             *
                             * mov      r1, #par_i
                             * str      r1, [r2, #-(stk+=4)]
                             * */
                            body << Instruction::MoveInstruction(r2, fp);
#warning "The following support for imm16 is not implemented."
                            if (par.getIrOpType() == IntermediateRepresentation::Var)
                                load_to_reg(r1, par.getVarName());
                            else
                                body << Instruction::MoveInstruction(r1, imm16());
                            body << Instruction::SaveInstruction(r1, Operands::LoadSaveOperand(r2, -(tmp_stk_size += 4),true));
                        }
                        // r0, r1, r2, r3
                        switch (std::min(4, arg_cnt)) {
                            case 4: {
                                auto &par = ops[5];
                                if (par.getIrOpType() == IntermediateRepresentation::ImmVal) {
                                    // imm16
                                    body << Instruction::MoveInstruction(r0, imm16(par.getValue()));
                                } else
                                    load_to_reg(r3, par.getVarName());
                            }
                            case 3: {
                                auto &par = ops[4];
                                if (par.getIrOpType() == IntermediateRepresentation::ImmVal) {
                                    // imm16
                                    body << Instruction::MoveInstruction(r0, imm16(par.getValue()));
                                } else
                                    load_to_reg(r2, par.getVarName());
                            }

                            case 2: {
                                auto &par = ops[3];
                                if (par.getIrOpType() == IntermediateRepresentation::ImmVal) {
                                    // imm16
                                    body << Instruction::MoveInstruction(r0, imm16(par.getValue()));
                                } else
                                    load_to_reg(r1, par.getVarName());
                            }

                            case 1: {
                                auto &par = ops[2];
                                if (par.getIrOpType() == IntermediateRepresentation::ImmVal) {
                                    // imm16
                                    body << Instruction::MoveInstruction(r0, imm16(par.getValue()));
                                } else
                                    load_to_reg(r0, par.getVarName());
                            }
                                break;
                            default:
                                throw std::runtime_error("Invalid call intermediate representation: too few arguments");
                        }
                        // b        func
                        body << Instruction::BranchInstruction(B, func.getVarName());
                        if (func.getIrDataType() != IntermediateRepresentation::t_void) {
                            // has a return value
                            if (ret.getIrOpType() != IntermediateRepresentation::Null && ret.getIrDataType() != IntermediateRepresentation::t_void) {
                                // return value will save into a variable
                                prepare_dest_var(ret);
                                save_to_stk(r0, ret.getVarName());
                            }
                        }
                    }
                        break;
                    case IntermediateRepresentation::RETURN: {
                        // func returns non-null:
                        //  mov     r0, <var>
                        //  b       LB_<funcName>_ret

                        // Type check:
                        if (function.getReturnType() != stmt.getDataType()) {
                            throw std::runtime_error("Invalid IR instruction: unmatched returned type: given " + std::to_string(stmt.getDataType())
                            + ", expecting " + std::to_string(function.getReturnType()));
                        }

                        if (function.getReturnType() != IntermediateRepresentation::t_void) {
                            if (ops.size() != 1)
                                throw std::runtime_error("Invalid IR instruction: invalid return arguments");
                            switch (ops[0].getIrOpType()) {
                                case IntermediateRepresentation::ImmVal:
                                    // mov      r0, #<val>
                                    body << Instruction::MoveInstruction(r0, imm16(ops[0].getValue()));
                                    break;
                                case IntermediateRepresentation::Var:
                                    // ldr      r0, <var>
                                    load_to_reg(r0, ops[0].getVarName());
                                    break;
                                case IntermediateRepresentation::Null:
                                    throw std::runtime_error("Invalid IR instruction: invalid argument type: Null");
                            }
                        }
                        body << Instruction::BranchInstruction(B, lb_pfx + "_ret");
                    }
                        break;
                    case IntermediateRepresentation::ALLOCA: {
                        auto& dest = ops.at(0).getVarName();
                        if (stk_mapping.count(dest))
                            throw std::runtime_error(std::string("Invalid IR variable '") + dest + std::string("': variable can only be allocated once."));
                        new_on_stk(dest);
                        setAlloca.insert(dest);
                    }
                        break;
                    case IntermediateRepresentation::LABEL: {
                        // .LB_funcName_<Label>:
                        Operands::Label label = lb_pfx + stmt.getLabelName();
                        body << LabelInstruction(label);
                        label_type[label] = 0; // common label
                        // label_map["<Label>"] = ".LB_funcName_<Label>";
                        label_map[stmt.getLabelName()] = label;
                    }
                        break;
                    case IntermediateRepresentation::LOAD: {
                        /*
                         * load_to_reg i32 %dest,  i32 %src
                         *
                         * %dest must be AllcaType or new!
                         * */
                        if (ops.size() != 2)
                            throw std::runtime_error("Invalid IR: unexpected arguments for LOAD statement");
                        auto& varName = ops[0].getVarName();
                        prepare_dest_var(ops[0]);
                        switch (ops[1].getIrOpType()) {
                            case IntermediateRepresentation::ImmVal: {
                                auto dataType = ops[1].getIrDataType();
                                if (dataType == IntermediateRepresentation::t_void || dataType == IntermediateRepresentation::str)
                                    throw std::runtime_error("Invalid IR: unexpected argument type for LOAD statement.");
                                int32_t imm_val = ops[1].getValue();
                                if (Utilities::isTooLong(imm_val)) {
                                    /*
                                     * .LB_<funcName>_TMP_<i>:
                                     *          .long   imm_val
                                     *          ldr     r0, .LB_<funcName>_TMP_<i>
                                     * */
                                    Operands::Label lb = lb_pfx + "TMP" + std::to_string(cnt++);
                                    body << LabelInstruction(lb)
                                         << DotInstruction(32, imm_val);
                                    label_type[lb] = 1;     // direct temporary
                                    load_to_reg(r0, lb);
                                } else {
                                    body << Instruction::MoveInstruction(r0, imm16(imm_val));
                                    if (ops[1].getIsPointer())
                                        body << Instruction::LoadInstruction(r0, Operands::LoadSaveOperand(r0));
                                }
                            }
                                break;
                            case IntermediateRepresentation::Var: {
                                /*
                                 * ldr      r0, <var_src>
                                 * */
                                load_to_reg(r0, ops[1].getVarName());

                                // storing normal variables: not a pointer
                                if (ops[1].getIsPointer()) {
                                    // This is a pointer: relative addressing.
                                    body << Instruction::LoadInstruction(r0, Operands::LoadSaveOperand(r0));
                                }
                            }
                                break;
                            case IntermediateRepresentation::Null:
                                throw std::runtime_error("Invalid IR: unexpected argument type for LOAD statement: Null");
                        }
                        // str      r0, <varName>
                        save_to_stk(r0, varName);
                    }
                        break;
                    case IntermediateRepresentation::STORE: {
                        /*
                         * store i32 %<source>, i32 %<dest>
                         * */
                        // only alloca
                        if (ops.size() != 2)
                            throw std::runtime_error("Invalid IR: unexpected arguments for STORE statement");
                        auto &dest = ops[1], &src = ops[0];
                        if (dest.getIrOpType() != IntermediateRepresentation::Var)
                            throw std::runtime_error("Invalid IR: unexpected type of an argument for STORE statement");
                        if (!setAlloca.count(src.getVarName())) // not an alloca
                            throw std::runtime_error("Invalid IR: destination of STORE statement must be an alloca-variable!");
                        switch (src.getIrOpType()) {
                            case IntermediateRepresentation::ImmVal: {
                                /*
                                 * ldr      r0, #imm
                                 * */
                                int32_t imm = src.getValue();
                                if (isTooLong(imm)) {
                                    /*
                                     * .LB_<funcName>_TMP_<i>:
                                     *          .long   imm_val
                                     *          ldr     r0, .LB_<funcName>_TMP_<i>
                                     * */
                                    Operands::Label lb = lb_pfx + "TMP" + std::to_string(cnt++);
                                    body << LabelInstruction(lb)
                                         << DotInstruction(32, imm);
                                    label_type[lb] = 1;     // direct temporary
                                    load_to_reg(r0, lb);
                                } else
                                    body << Instruction::MoveInstruction(r0, imm16(imm));
                            }
                                break;
                            case IntermediateRepresentation::Var:
                                /*
                                 * ldr      r0, <src>
                                 * */
                                load_to_reg(r0, src.getVarName());
                                break;
                            case IntermediateRepresentation::Null:
                                throw std::runtime_error("Invalid IR: unexpected arguments for STORE statement");
                        }
                        save_to_stk(r0, dest.getVarName());
                    }
                        break;
                    case IntermediateRepresentation::CMP_EQ: {
                        /*
                         * -> cmp_eq i1, %1, i32 %2, i32 %3
                         * load_to_reg(r0, "1")
                         * cmp      r0, <imm_val>
                         *
                         * Remember to set cmpFlag to 'Comp_Equal' !
                         * */
                        auto& source1 = ops[1], & source2 = ops[2];
                        create_cmp_instruction(source1, source2);
                        cmpFlag = Instruction::Condition::Cond_Equal;
                    }
                        break;
                    case IntermediateRepresentation::CMP_NE: {
                        auto& source1 = ops[1], & source2 = ops[2];
                        create_cmp_instruction(source1, source2);
                        cmpFlag = Instruction::Condition::Cond_NotEqual;
                    }
                        break;
                    case IntermediateRepresentation::CMP_SGE: {
                        auto& source1 = ops[1], & source2 = ops[2];
                        create_cmp_instruction(source1, source2);
                        cmpFlag = Instruction::Condition::Cond_SGreaterEqual;
                    }
                        break;
                    case IntermediateRepresentation::CMP_SLE: {
                        auto& source1 = ops[1], & source2 = ops[2];
                        create_cmp_instruction(source1, source2);
                        cmpFlag = Instruction::Condition::Cond_SLessEqual;
                    }
                        break;
                    case IntermediateRepresentation::CMP_SGT: {
                        auto& source1 = ops[1], & source2 = ops[2];
                        create_cmp_instruction(source1, source2);
                        cmpFlag = Instruction::Condition::Cond_SGreater;
                    }
                        break;
                    case IntermediateRepresentation::CMP_SLT: {
                        auto& source1 = ops[1], & source2 = ops[2];
                        create_cmp_instruction(source1, source2);
                        cmpFlag = Instruction::Condition::Cond_SLess;
                    }
                        break;
                    case IntermediateRepresentation::GLB_CONST:
                    case IntermediateRepresentation::GLB_VAR: {
                        throw std::runtime_error("Global declarations should not appear in function body");
                    }
                    case IntermediateRepresentation::LSH: {

                    }
                        break;
                    case IntermediateRepresentation::RSH: {

                    }
                        break;
                    case IntermediateRepresentation::OR: {

                    }
                        break;
                    case IntermediateRepresentation::AND: {

                    }
                        break;
                    case IntermediateRepresentation::XOR: {

                    }
                        break;
                    case IntermediateRepresentation::NOT: {

                    }
                        break;
                    case IntermediateRepresentation::PHI: {

                    }
                        break;
                }
            }

            // move stack pointer
            //      sub     sp, sp, #<stack_size>
            front_wrapper << Instruction::SubtractionInstruction(sp, sp, Operands::ImmediateNumber<12> (stack_size));

            /* end_wrapper
             *      // if fo_off != 0:
             *      add     sp, sp, #<stack_size>+sp_off;
             *      // else:
             *      mov     sp, fp
             *      pop     { fp, lr }
             *      bx      lr
             * */
            if (!fp_off)
                end_wrapper << Instruction::MoveInstruction(sp, fp);
            else
                end_wrapper << Instruction::AdditionInstruction(sp, sp, imm12(stack_size + sp_off));
            end_wrapper << Instruction::PopInstruction(Operands::RegisterList(FP, LR))
                        << Instruction::BranchInstruction(BX, lr);
            appendAns(front_wrapper);
            appendAns(body);
            appendAns(end_wrapper);
        }
    public:
        Instruction::InstructionStream doTranslation() override {
            // process globals
            loadHeap();
            // process functions
            auto& functions = irProgram.getFunctions();
            for (auto & func : functions)
                processFunction(func);
            return ans;
        }
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
//                    st_it = stmts.insert(st_it, { IntermediateRepresentation::PARAM, IntermediateRepresentation::i32, *rit, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, pos--) } ) + 1;
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
                            // param       [ %<func>_arg_placeholder_x, null ]
                            paramOpr.push_back(tmpOpr);
                        }
                    }
                    it = stmts.insert(it, { IntermediateRepresentation::PARAM, IntermediateRepresentation::i32, paramOpr }) + 1;

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
                // push { rx, rx, ..., fp, lr, sp }
                list.emplaceRegister(fp, lr, sp);
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
                                auto breq = BranchInstruction(B, ops[0].getStrValue());
                                breq.setCondition(Instruction::Condition::Cond_Equal);
                                ins << std::move(breq);
                                ins << BranchInstruction(B, ops[1].getStrValue());
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
                            // pop      { rx, rx, ..., fp, lr, sp }
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
                            ins << SaveInstruction(mapping.at(ops[0]), Operands::LoadSaveOperand(lr, ops[1].getValue(),
                                                                                              true));
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
