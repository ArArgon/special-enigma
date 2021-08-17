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
#include <sstream>

#include "InstructionOperands.h"
#include "InstructionUtilities.h"

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
    enum ShiftType {
        ASR = 0, LSL, LSR, ROR, RRX
    };
    const std::string shiftASM[] = {
            "asr", "lsl", "lsr", "ror", "rrx"
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
                case Cond_Equal:
                    return "eq";
                case Cond_NotEqual:
                    return "ne";
                case Cond_SGreaterEqual:
                    return "ge";
                case Cond_SLessEqual:
                    return "le";
                case Cond_SGreater:
                    return "gt";
                case Cond_SLess:
                    return "lt";
                case Cond_UGreaterEqual:
                    return "hs";
                case Cond_ULessEqual:
                    return "ls";
                case Cond_UGreater:
                    return "hi";
                case Cond_ULess:
                    return "lo";
            }
            throw std::runtime_error("Invalid IR: unexpected condition: " + std::to_string(cond));
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

        template<typename insT = MachineInstruction>
        friend InstructionStream &operator+= (InstructionStream& is, const insT& ins) {
            return is << ins;
        }
    };

    class DotInstruction : public MachineInstruction {
    public:
        enum DotType {
            BYTE, BYTE_2, BYTE_4, BYTE_8, ASCII, ASCIZ, LONG, WORD, ZERO, GLOBL, TEXT, DATA, END, CUSTOM
        };
    private:
        DotType dotType;
        uint32_t bValue;
        bool isStr;
        std::string sValue, customDot;
        static constexpr DotType byte_to_dir[] = {
                BYTE, BYTE, BYTE_2, BYTE_4, BYTE_4, BYTE_8, BYTE_8, BYTE_8, BYTE_8
        };
    public:
        DotInstruction(DotType dotType, const std::string &sValue) : dotType(dotType), sValue(sValue), isStr(true) { }

        DotInstruction(DotType dotType, uint32_t bValue, bool isStr) : dotType(dotType), bValue(bValue), isStr(false) { }

        DotInstruction(std::string customDot, std::string sValue) : dotType(CUSTOM), customDot(customDot), sValue(sValue), isStr(true) { }

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
            std::string ins;
            switch (dotType) {
                case BYTE:
                    ins = ".byte";
                    break;
                case BYTE_2:
                    ins = ".2byte";
                    break;
                case BYTE_4:
                    ins = ".4byte";
                    break;
                case BYTE_8:
                    ins = ".8byte";
                    break;
                case ASCII:
                    ins = ".ascii";
                    break;
                case ASCIZ:
                    ins = ".asciz";
                    break;
                case LONG:
                    ins = ".long";
                    break;
                case WORD:
                    ins = ".word";
                    break;
                case ZERO:
                    ins = ".zero";
                    break;
                case GLOBL:
                    ins = ".global";
                    break;
                case TEXT:
                    ins = ".text";
                    break;
                case DATA:
                    ins = ".data";
                    break;
                case END:
                    ins = ".end";
                    break;
                case CUSTOM:
                    ins = "." + customDot;
                    break;
            }
            std::string opr;
            if (isStr) {
                opr = sValue;
                if (dotType == ASCII || dotType == ASCIZ) {
                    opr.insert(opr.begin(), '"');
                    opr.insert(opr.end(), '"');
                }
            } else
                opr = std::to_string(bValue);
            return Utilities::ASMFormatter(ins, opr).toASM();
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
            return Utilities::ASMFormatter(labelName + ":").toASM();
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
            std::string ins, opr;
            switch (branchType) {
                case B:
                    ins += "b";
                    break;
                case BL:
                    ins += "bl";
                    break;
                case BLX:
                    ins += "blx";
                    break;
                case BX:
                    ins += "bx";
                    break;
                default:
                    throw std::runtime_error(std::string("Unexpected branch type: ") + std::to_string(branchType));
            }
            ins += condition.toASM();
            opr = toRegister ? targetRegister.toASM() : targetLabel;
            return Utilities::ASMFormatter(ins, opr).toASM();
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
            std::string ins = asm_name() + condition.toASM(), opr;
            if (update)
                ins += "s";
            const Operands::Operand* source_2 = isImmediate ? static_cast<const Operands::Operand *>(&source_2_imm)
                                                            : static_cast<const Operands::Operand *>(&source_2_flex);
            opr = target.toASM() + ", " + source_1.toASM() + ", " + source_2->toASM();
            return Utilities::ASMFormatter(ins, opr).toASM();
        }
    };

    class AdditionInstruction : public ArithmeticProto {
        using ArithmeticProto::ArithmeticProto;
        std::string asm_name() const override {
            return "add";
        }
    };

    class
    SubtractionInstruction : public ArithmeticProto {
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

    class ShiftInstruction : public MachineInstruction {
        Operands::Register target, source, bits;
        ShiftType type;
        size_t immBits;
        bool useRegister;

    public:
        ShiftInstruction(ShiftType type, Operands::Register target, Operands::Register source,
                         Operands::Register bits) : target(std::move(target)), source(std::move(source)), bits(std::move(bits)), useRegister(true), type(type) { }

        ShiftInstruction(ShiftType type, Operands::Register target, Operands::Register source, size_t immBits) : target(std::move(
                target)), source(std::move(source)), immBits(immBits), useRegister(false), type(type) { }

    public:
        std::string toASM() const override {
            std::string ins = shiftASM[type] + condition.toASM(), opr;
            opr = target.toASM() + ", " + source.toASM() + ", ";
            if (useRegister)
                opr += bits.toASM();
            else
                opr += "#" + std::to_string(immBits);
            return Utilities::ASMFormatter(ins, opr).toASM();
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

            std::string ins = bitTable[instructionType] + condition.toASM();
            if (update)
                ins += "s";
            return Utilities::ASMFormatter(ins, target.toASM() + ", " + source_1.toASM() + ", " + source_2.toASM()).toASM();
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

            return Utilities::ASMFormatter(bitTable[instructionType] + condition.toASM(), source_1.toASM() + ", " + source_2.toASM()).toASM();
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
            return Utilities::ASMFormatter("push", registerList.toASM()).toASM();
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
            return Utilities::ASMFormatter("pop", registerList.toASM()).toASM();
        }
    };

    class MoveInverseInstruction : public MachineInstruction {
        Operands::Register destReg;
        Operands::Operand2 resOpr2;
        bool update;

    public:
        MoveInverseInstruction(const Operands::Register &destReg, const Operands::Operand2 &resOpr2, bool update = false)
                : destReg(destReg), resOpr2(resOpr2), update(update) { }

        std::string toASM() const override {
            return Utilities::ASMFormatter("mvn" + condition.toASM() + (update ? std::string("s") : std::string("")), destReg.toASM() + ", " + resOpr2.toASM()).toASM();
        }

    };

    class MoveInstruction : public MachineInstruction {
    public:
        enum MovePosition {
            ALL = 0, HIGH, LOW
        };
    private:
        Operands::Register destReg;
        Operands::Operand2 resOpr2;
        Operands::ImmediateNumber<16> resImm16;
        bool isOpr2, update;
        MovePosition position = ALL;

    public:

        void setPosition(MovePosition position) {
            MoveInstruction::position = position;
        }

        MovePosition getPosition() const {
            return position;
        }

        MoveInstruction(const Operands::Register &sourceReg, const Operands::Operand2 &destOpr2) : destReg(sourceReg), update(false),
                                                                                                   resOpr2(destOpr2), isOpr2(true) { }

        MoveInstruction(const Operands::Register &sourceReg, const Operands::ImmediateNumber<16> &destImm16)
                : destReg(sourceReg), resImm16(destImm16), isOpr2(false), update(false) { }

        MoveInstruction(const Operands::Register &destReg, const Operands::Operand2 &resOpr2, bool update) : destReg(
                destReg), resOpr2(resOpr2), update(update), isOpr2(true) { }

        MoveInstruction(const Operands::Register &destReg, const Operands::ImmediateNumber<16> &resImm16, bool update)
                : destReg(destReg), resImm16(resImm16), update(update), isOpr2(false) { }

        MoveInstruction(const Operands::Register &destReg, const Operands::ImmediateNumber<16> &resImm16, bool update, MovePosition position)
                : destReg(destReg), resImm16(resImm16), update(update), isOpr2(false), position(position) { }

        Operands::Register getTarget() const {
            return destReg;
        }

        Operands::Operand2 getSource() const {
            return resOpr2;
        }

        std::string toASM() const override {
            std::string ins = "mov", opr;
            if (position == HIGH)
                ins += "t";
            else if (position == LOW)
                ins += "w";
            ins += condition.toASM();
            ins += update ? "s" : "";
            opr = destReg.toASM() + ", ";
            if (isOpr2)
                opr += resOpr2.toASM();
            else
                opr += resImm16.toASM();
            return Utilities::ASMFormatter(ins, opr).toASM();
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

        Operands::Register getTarget() const {
            return target;
        }

        Operands::LoadSaveOperand getSource() const {
            return source;
        }

        std::string toASM() const override {
            std::string ins = protoName();
            switch (bitSize) {
                case bit_DEF:
                    break;
                case bit_B:
                    ins += "b";
                    break;
                case bit_SB:
                    ins += "sb";
                    break;
                case bit_H:
                    ins += "h";
                    break;
                case bit_SH:
                    ins += "sh";
                    break;
            }
            return Utilities::ASMFormatter(ins, target.toASM() + ", " + source.toASM()).toASM();
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
            bitSize = bit_DEF;
        }

        LoadInstruction(Operands::Register targetReg, int immVal) {
            destIsLabel = true;
            LoadSaveProto::target = std::move(targetReg);
            label = "=" + std::to_string(immVal);
            bitSize = bit_DEF;
        }

        std::string protoName() const override {
            return std::string("ldr");
        }

        std::string toASM() const override {
            if (destIsLabel) {
                std::string ins = "ldr";
                switch (bitSize) {
                    case bit_DEF:
                        break;
                    case bit_B:
                        ins += "b";
                        break;
                    case bit_SB:
                        ins += "sb";
                        break;
                    case bit_H:
                        ins += "h";
                        break;
                    case bit_SH:
                        ins += "sh";
                        break;
                }
                return Utilities::ASMFormatter(ins, target.toASM() + ", " + label).toASM();
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
