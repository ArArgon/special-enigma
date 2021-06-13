//
// Created by 廖治平 on 5/30/21.
//

#ifndef SYSYBACKEND_IRTRANSLATOR_H
#define SYSYBACKEND_IRTRANSLATOR_H

#include <unordered_map>
#include <utility>
#include <stack>
#include <set>
#include <functional>

#include "Instruction.h"
#include "IRTypes.h"

namespace Backend::Translator {
    // Base class for translator
    using namespace Instruction::Utilities;
    using namespace Instruction::Utilities::Abbr;
    using namespace Instruction;

    bool enable_direct_label_write = false;

    class Translator {
    protected:
        IntermediateRepresentation::IRProgram irProgram;

    public:
        explicit Translator(IntermediateRepresentation::IRProgram irProgram) : irProgram(std::move(irProgram)) { }
        virtual Instruction::InstructionStream doTranslation() = 0;
        virtual ~Translator() = default;
    };

    class SimpleTranslator : public Translator {
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
         *          eg2: label_type["LB_<funcName>_tmp_1"] = 1;// temporary or direct label
         *          eg3: label_type["dest"] = 2;                // static or relative label
         *          eg4: label_type["LB_GLB_2"] = 1;           // temporary or direct label
         *          eg5: label_type["LB_STR_1"] = 1;           // temporary or direct label
         * */

        std::unordered_map<std::string, int> label_type;
        std::unordered_map<std::string, std::string> label_map,     // mapping labels to their modified name
                                                     relative_map;  // mapping static vars to their relative label
        Instruction::InstructionStream ans, global;

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

            auto create_cmp_instruction = [&](const auto& source1, const auto& source2) {
                switch (source1.getIrOpType()) {
                case IntermediateRepresentation::ImmVal:
                    //ERROR
                    break;
                case IntermediateRepresentation::Var:
                    load_to_reg(r0, source1.getVarName());
                    break;
                case IntermediateRepresentation::Null:
                    throw std::runtime_error("Invalid IR: unexpected arguments for ICMP        statement");
                    break;
                }
                switch (source2.getIrOpType()) {
                case IntermediateRepresentation::ImmVal:
                    int32_t imm = source2.getValue();
                    body << Instruction::ComparisonInstruction(Instruction::CMP, r0, imm16(imm));
                    break;
                case IntermediateRepresentation::Var:
                    load_to_reg(r1, source2.getVarName());
                    body << Instruction::ComparisonInstruction(Instruction::CMP, r0, r1);
                    break;
                case IntermediateRepresentation::Null:
                    throw std::runtime_error("Invalid IR: unexpected arguments for ICMP        statement");
                    break;
                }
            }

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
                        int arg_cnt = ops.size() - 2;
                        if (arg_cnt < 0)
                            throw std::runtime_error("Invalid call intermediate representation: too few arguments");
                        if (data_type == IntermediateRepresentation::t_void) {
                            // no return
                            // save_to_stk
                            for (int i = 1; i <= arg_cnt; i++) {

                            }
                        } else {

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
                                    break;
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
                                break;
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
                                break;
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
                        break;
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
}

#endif //SYSYBACKEND_IRTRANSLATOR_H
