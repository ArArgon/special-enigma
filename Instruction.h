//
// Created by 廖治平 on 5/22/21.
//

#ifndef SYSYBACKEND_INSTRUCTION_H
#define SYSYBACKEND_INSTRUCTION_H

#include <utility>
#include <vector>
#include <array>
#include <memory>
#include <string>
#include <ostream>

#include "InstructionOperands.h"

namespace Instruction {
    // Enumerations
    enum BrType {
        B, BL, BLX, BX
    };
    enum BitType {
        AND, EOR, ORR, ORN, BIC
    };
    enum CMPType {
        CMP, CMN, TST, TEQ
    };

    enum DataSegType {
        BYTE1, BYTE2, STR, SKIP
    };

    class Condition {
    public:
        enum Cond {
            Cond_NO, 
            Cond_Equal, Cond_NotEqual,
            Cond_SGreaterEqual, Cond_SLessEqual,
            Cond_SGreater, Cond_SLess,
            Cond_UGreaterEqual, Cond_ULessEqual,
            Cond_UGreater, Cond_ULess,
        };
    private:
        Cond cond;
    public:
        Condition() : cond(Cond_NO) { }
        explicit Condition(Cond cond) : cond(cond) { }

        Cond getCond() const {
            return cond;
        }

        void setCond(Cond cond) {
            Condition::cond = cond;
        }

        Condition& operator= (const Cond cond) {
            this->cond = cond;
            return *this;
        }

        std::string toASM() const {
            switch (cond) {
                case Cond_NO:
                    return "";
                    break;
                case Cond_Equal:
                    return "eq";
                    break;
                case Cond_NotEqual:
                    return "ne";
                    break;
                case Cond_SGreaterEqual:
                    return "ge";
                    break;
                case Cond_SLessEqual:
                    return "le";
                    break;
                case Cond_SGreater:
                    return "gt";
                    break;
                case Cond_SLess:
                    return "lt";
                    break;
                case Cond_UGreaterEqual:
                    return "hs";
                    break;
                case Cond_ULessEqual:
                    return "ls";
                    break;
                case Cond_UGreater:
                    return "hi";
                    break;
                case Cond_ULess:
                    return "lo";
                    break;
            }
        }
    };

    // Instruction base
    class MachineInstruction {
        typedef std::vector<std::shared_ptr<MachineInstruction>> InstructionStream;
    protected:
        Condition condition;
    public:

        const Condition &getCondition() const {
            return condition;
        }

        void setCondition(Condition::Cond cond) {
            MachineInstruction::condition = cond;
        }

        virtual std::string toASM() const = 0;
        virtual ~MachineInstruction() = default;

        friend std::ostream &operator<<(std::ostream &os, const MachineInstruction &instruction) {
            os << instruction.toASM();
            return os;
        }

        template<typename insT = MachineInstruction>
        friend InstructionStream &operator<< (InstructionStream& is, const insT&& instruction) {
            is.push_back(std::make_shared<insT>(instruction));
            return is;
        }

//        template<typename insT>
//        friend InstructionStream &operator<< (InstructionStream& is, const insT& instruction) {
//            is.push_back(std::make_shared<insT>(instruction));
//            return is;
//        }

        template<typename insT = MachineInstruction>
        friend InstructionStream &operator+= (InstructionStream& is, const insT& ins) {
            return is << ins;
        }
    };

    class DotInstruction : public MachineInstruction {
    public:
        enum DotType {
            BYTE, BYTE_2, BYTE_4, BYTE_8, ASCII, ASCIZ, LONG, WORD
        };
    private:
        DotType dotType;
        uint32_t bValue;
        bool isStr;
        std::string sValue;
        static constexpr DotType byte_to_dir[] = {
                BYTE, BYTE, BYTE_2, BYTE_4, BYTE_4, BYTE_8, BYTE_8, BYTE_8, BYTE_8
        };
    public:
        DotInstruction(DotType dotType, const std::string &sValue) : dotType(dotType), sValue(sValue), isStr(true) { }

        DotInstruction(size_t len, uint64_t bValue) : bValue(bValue) {
            isStr = false;
            if (len > 8)
                throw std::runtime_error("Invalid byte value: longer than 8");
            dotType = byte_to_dir[bValue];
        }

        DotInstruction(std::string sValue) : sValue(std::move(sValue)), dotType(ASCIZ), isStr(true) { }

        DotType getDirectiveType() const {
            return dotType;
        }

        void setDirectiveType(DotType directiveType) {
            DotInstruction::dotType = directiveType;
        }

        uint64_t getBValue() const {
            return bValue;
        }

        void setBValue(uint64_t bValue) {
            DotInstruction::bValue = bValue;
        }

        const std::string &getSValue() const {
            return sValue;
        }

        void setSValue(const std::string &sValue) {
            DotInstruction::sValue = sValue;
        }

        std::string toASM() const override {
            std::string ans;
            switch (dotType) {
                case BYTE:
                    ans = std::string(".byte    ");
                    break;
                case BYTE_2:
                    ans = std::string(".2byte   ");
                    break;
                case BYTE_4:
                    ans = std::string(".4byte   ");
                    break;
                case BYTE_8:
                    ans = std::string(".8byte   ");
                    break;
                case ASCII:
                    ans = std::string(".ascii   ");
                    break;
                case ASCIZ:
                    ans = std::string(".asciz   ");
                    break;
                case LONG:
                    ans = std::string(".long    ");
                    break;
                case WORD:
                    ans = std::string(".word    ");
                    break;
            }
            if (isStr)
                ans += std::to_string(bValue);
            else
                ans += "\"" + sValue + "\"";
            return ans;
        }
    };

    class LabelInstruction : public MachineInstruction {
        Operands::Label labelName;

    public:
        LabelInstruction(Operands::Label labelName) : labelName(std::move(labelName)) { }

        const std::string &getLabelName() const {
            return labelName;
        }

        void setLabelName(const Operands::Label &labelName) {
            LabelInstruction::labelName = labelName;
        }

        std::string toASM() const override  {
            return labelName + ":";
        }
    };

    class DataInstruction : public MachineInstruction {

    };

    class BranchInstruction: public MachineInstruction {
    private:
        // 0: b, 1: bl, 2: bx
        BrType branchType;
        bool toRegister;
        Operands::Register targetRegister;
        Operands::Label targetLabel;

    public:

        BranchInstruction(BrType branchType, Operands::Label target) : branchType(branchType), toRegister(false),
                                                                       targetLabel(std::move(target)) { }

        BranchInstruction(BrType branchType, Operands::Register targetRegister) : branchType(branchType), toRegister(true),
                                                                                  targetRegister(std::move(targetRegister)) { }

        BrType getBranchType() const {
            return branchType;
        }

        const Operands::Register &getTargetRegister() const {
            return targetRegister;
        }

        void setTargetRegister(const Operands::Register &targetRegister) {
            BranchInstruction::targetRegister = targetRegister;
        }

        void setBranchType(BrType branchType) {
            BranchInstruction::branchType = branchType;
        }

        const std::string &getTargetLabel() const {
            return targetLabel;
        }

        void setTargetLabel(const std::string &toTarget) {
            BranchInstruction::targetLabel = toTarget;
        }

        std::string toASM() const override  {
            std::string ans = "b";
            switch (branchType) {
                case B:
                    ans += " ";
                    break;
                case BL:
                    ans += "l ";
                    break;
                case BLX:
                    ans += "lx ";
                case BX:
                    ans += "x ";
                default:
                    throw std::runtime_error(std::string("Unexpected branch type: ") + std::to_string(branchType));
            }
            if (toRegister)
                return ans + targetRegister.toASM();
            return ans + targetLabel;
            return ans + targetLabel;
        }
    };

    class ArithmeticProto : public MachineInstruction {
        Operands::Register target, source_1;
        Operands::FlexibleOperand source_2_flex;
        Operands::ImmediateNumber<12> source_2_imm;
        bool update;
        bool isImmediate;
    protected:
        virtual std::string asm_name() const = 0;
    public:

        ArithmeticProto(Operands::Register target, Operands::Register source1,
                        Operands::FlexibleOperand source2) : target(std::move(target)), source_1(std::move(source1)),
                                                                        source_2_flex(std::move(source2)), update(false),
                                                                        isImmediate(false) { }

        ArithmeticProto(Operands::Register target, Operands::Register source1,
                        Operands::ImmediateNumber<12> source2Imm) : target(std::move(target)), source_1(std::move(source1)),
                                                                               source_2_imm(std::move(source2Imm)),
                                                                               isImmediate(true), update(false) { }

        bool isUpdate() const {
            return update;
        }

        void setUpdate(bool update) {
            ArithmeticProto::update = update;
        }

        const Operands::Register &getTarget() const {
            return target;
        }

        void setTarget(const Operands::Register &target) {
            ArithmeticProto::target = target;
        }

        const Operands::Register &getSource1() const {
            return source_1;
        }

        void setSource1(const Operands::Register &source1) {
            source_1 = source1;
        }

        const Operands::FlexibleOperand &getSource2Flex() const {
            return source_2_flex;
        }

        void setSource2Flex(const Operands::FlexibleOperand &source2) {
            source_2_flex = source2;
        }

        std::string toASM() const override  {
            std::string ans = asm_name();
            if (update)
                ans += "s";
            const Operands::Operand* source_2 = isImmediate ? static_cast<const Operands::Operand *>(&source_2_imm)
                                                            : static_cast<const Operands::Operand *>(&source_2_flex);
            ans += " " + target.toASM() + ", " + source_1.toASM() + ", " + source_2->toASM();
            return ans;
        }
    };

    class AdditionInstruction : public ArithmeticProto {
        using ArithmeticProto::ArithmeticProto;
        std::string asm_name() const override {
            return "add";
        }
    };

    class SubtractionInstruction : public ArithmeticProto {
        using ArithmeticProto::ArithmeticProto;
        std::string asm_name() const override {
            return "sub";
        }
    };

    class MultiplicationInstruction : public ArithmeticProto {
        using ArithmeticProto::ArithmeticProto;
        std::string asm_name() const override {
            return "mul";
        }
    };

    class BitInstruction : public MachineInstruction {
    private:
        BitType instructionType;
        Operands::Register target, source_1;
        Operands::FlexibleOperand source_2;
        bool update;

    public:
        BitInstruction(BitType instructionType, Operands::Register target, Operands::Register source1,
                       Operands::FlexibleOperand source2, bool isUpdate = false) : instructionType(instructionType), target(std::move(target)),
                                                         source_1(std::move(source1)), source_2(std::move(source2)), update(isUpdate) { }

        BitType getInstructionType() const {
            return instructionType;
        }

        void setInstructionType(BitType instructionType) {
            BitInstruction::instructionType = instructionType;
        }

        bool isUpdate() const {
            return update;
        }

        void setUpdate(bool update) {
            BitInstruction::update = update;
        }

        const Operands::Register &getTarget() const {
            return target;
        }

        void setTarget(const Operands::Register &target) {
            BitInstruction::target = target;
        }

        const Operands::Register &getSource1() const {
            return source_1;
        }

        void setSource1(const Operands::Register &source1) {
            source_1 = source1;
        }

        const Operands::FlexibleOperand &getSource2() const {
            return source_2;
        }

        void setSource2(const Operands::FlexibleOperand &source2) {
            source_2 = source2;
        }

        std::string toASM() const override  {
            const static std::string bitTable[] {
                "and", "eor", "orr", "orn", "bic"
            };

            if (instructionType > BitType::BIC)
                throw std::runtime_error(std::string("Unexpected bit instruction type: ") + std::to_string(instructionType));

            std::string ins = bitTable[instructionType];
            if (update)
                ins += "s";
            return ins + " " + target.toASM() + ", " + source_1.toASM() + ", " + source_2.toASM();
        }
    };

    class ComparisonInstruction : public MachineInstruction {
    private:
        CMPType instructionType;
        Operands::Register source_1;
        Operands::FlexibleOperand source_2;

    public:
        ComparisonInstruction(CMPType instructionType, const Operands::Register &source1,
                              const Operands::FlexibleOperand &source2) : instructionType(instructionType), source_1(source1),
                                                                source_2(source2) { }

        CMPType getInstructionType() const {
            return instructionType;
        }

        void setInstructionType(CMPType instructionType) {
            ComparisonInstruction::instructionType = instructionType;
        }

        const Operands::Register &getSource1() const {
            return source_1;
        }

        void setSource1(const Operands::Register &source1) {
            source_1 = source1;
        }

        const Operands::FlexibleOperand &getSource2() const {
            return source_2;
        }

        void setSource2(const Operands::FlexibleOperand &source2) {
            source_2 = source2;
        }

        std::string toASM() const override  {
            const static std::string bitTable[] {
                    "cmp", "cmn", "tst", "teq"
            };

            if (instructionType > CMPType::TEQ)
                throw std::runtime_error(std::string("Unexpected comparison instruction type: ") + std::to_string(instructionType));

            return bitTable[instructionType] + " " + source_1.toASM() + ", " + source_2.toASM();
        }

    };

    class PushInstruction : public MachineInstruction {
        Operands::RegisterList registerList;

    public:
        PushInstruction(Operands::RegisterList registerList) : registerList(std::move(registerList)) { }

        const Operands::RegisterList &getRegisterList() const {
            return registerList;
        }

        void setRegisterList(const Operands::RegisterList &registerList) {
            PushInstruction::registerList = registerList;
        }

        std::string toASM() const override {
            return std::string("push " + registerList.toASM());
        }
    };

    class PopInstruction : public MachineInstruction {
        Operands::RegisterList registerList;

    public:
        PopInstruction(const Operands::RegisterList &registerList) : registerList(registerList) { }

        const Operands::RegisterList &getRegisterList() const {
            return registerList;
        }

        void setRegisterList(const Operands::RegisterList &registerList) {
            PopInstruction::registerList = registerList;
        }

        std::string toASM() const override {
            return std::string("pop " + registerList.toASM());
        }
    };

    class MoveInstruction : public MachineInstruction {
        Operands::Register destReg;
        Operands::Operand2 resOpr2;
        Operands::ImmediateNumber<16> resImm16;
        bool isOpr2, update;

    public:

        MoveInstruction(const Operands::Register &sourceReg, const Operands::Operand2 &destOpr2) : destReg(sourceReg), update(false),
                                                                                                   resOpr2(destOpr2), isOpr2(true) { }

        MoveInstruction(const Operands::Register &sourceReg, const Operands::ImmediateNumber<16> &destImm16)
                : destReg(sourceReg), resImm16(destImm16), isOpr2(false), update(false) { }

        MoveInstruction(const Operands::Register &destReg, const Operands::Operand2 &resOpr2, bool update) : destReg(
                destReg), resOpr2(resOpr2), update(update), isOpr2(true) { }

        MoveInstruction(const Operands::Register &destReg, const Operands::ImmediateNumber<16> &resImm16, bool update)
                : destReg(destReg), resImm16(resImm16), update(update), isOpr2(false) {}

        std::string toASM() const override {
            std::string ans = "mov";
            ans += update ? "s " : " ";
            ans += destReg.toASM();
            if (isOpr2)
                ans += resOpr2.toASM();
            else
                ans += resImm16.toASM();
            return ans;
        }
    };

    class LoadSaveProto : public MachineInstruction {
    public:
        enum BitSize {
            bit_DEF, bit_B, bit_SB, bit_H, bit_SH
        };
    protected:
        BitSize bitSize;
        Operands::Register target;
        Operands::LoadSaveOperand source;
        virtual std::string protoName() const = 0;
    public:
        LoadSaveProto() = default;

        LoadSaveProto(Operands::Register target, Operands::LoadSaveOperand source) : target(std::move(target)),
                                                                                                   source(std::move(source)),
                                                                                                   bitSize(bit_DEF) { }

        LoadSaveProto(BitSize bitSize, Operands::Register target, Operands::LoadSaveOperand source)
                : bitSize(bitSize), target(std::move(target)), source(std::move(source)) { }

        std::string toASM() const override {
            std::string ans = protoName();
            switch (bitSize) {
                case bit_DEF:
                    ans += " ";
                    break;
                case bit_B:
                    ans += "b ";
                    break;
                case bit_SB:
                    ans += "sb ";
                    break;
                case bit_H:
                    ans += "h ";
                    break;
                case bit_SH:
                    ans += "sh ";
                    break;
            }
            ans += target.toASM() + ", " + source.toASM();
            return ans;
        }
    };

    class LoadInstruction : public LoadSaveProto {
    private:
        Operands::Label label;
        bool destIsLabel = false;
    public:
        using LoadSaveProto::LoadSaveProto;

        LoadInstruction(Operands::Register targetReg, Operands::Label label) : label(std::move(label)), destIsLabel(true) {
            LoadSaveProto::target = std::move(targetReg);
        }

        std::string protoName() const override {
            return std::string("ldr");
        }

        std::string toASM() const override {
            if (destIsLabel) {
                std::string ans = "ldr";
                switch (bitSize) {
                    case bit_DEF:
                        ans += " ";
                        break;
                    case bit_B:
                        ans += "b ";
                        break;
                    case bit_SB:
                        ans += "sb ";
                        break;
                    case bit_H:
                        ans += "h ";
                        break;
                    case bit_SH:
                        ans += "sh ";
                        break;
                }
                ans += target.toASM() + ", " + label;
                return ans;
            } else
                return LoadSaveProto::toASM();
        }
    };

    class SaveInstruction : public LoadSaveProto {
        std::string protoName() const override {
            return std::string("str");
        }
    public:
        using LoadSaveProto::LoadSaveProto;
    };

    typedef std::vector<std::shared_ptr<MachineInstruction>> InstructionStream;
}

#endif //SYSYBACKEND_INSTRUCTION_H
