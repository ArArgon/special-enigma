//
// Created by 廖治平 on 5/22/21.
//

#ifndef SYSYBACKEND_INSTRUCTION_H
#define SYSYBACKEND_INSTRUCTION_H

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

    class Condition {

    };

    // Instruction base
    class MachineInstruction {
        typedef std::vector<std::shared_ptr<MachineInstruction>> InstructionStream;
    protected:
        Condition condition;
        bool isConditioned;
    public:
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

    class LabelInstruction : public MachineInstruction {
        Operands::Label labelName;

    public:
        LabelInstruction(const Operands::Label &labelName) : labelName(labelName) { }

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

    class BranchInstruction: public MachineInstruction {
    private:
        // 0: b, 1: bl, 2: bx
        BrType branchType;
        Operands::Register targetRegister;
        Operands::Label target;

    public:

        BranchInstruction(BrType branchType, Operands::Label target) : branchType(branchType),
                                                                       target(std::move(target)) { }

        BrType getBranchType() const {
            return branchType;
        }

        void setBranchType(BrType branchType) {
            BranchInstruction::branchType = branchType;
        }

        const std::string &getTarget() const {
            return target;
        }

        void setTarget(const std::string &toTarget) {
            BranchInstruction::target = toTarget;
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
            return ans + target;
        }
    };

    class AdditionInstruction : public MachineInstruction {
        Operands::Register target, source_1;
        Operands::FlexibleOperand source_2_flex;
        Operands::ImmediateNumber<12> source_2_imm;
        bool update;
        bool isImmediate;
    public:

        AdditionInstruction(Operands::Register target, Operands::Register source1,
                            Operands::FlexibleOperand source2) : target(std::move(target)), source_1(std::move(source1)),
                                                                        source_2_flex(std::move(source2)), update(false),
                                                                        isImmediate(false) { }

        AdditionInstruction(Operands::Register target, Operands::Register source1,
                            Operands::ImmediateNumber<12> source2Imm) : target(std::move(target)), source_1(std::move(source1)),
                                                                               source_2_imm(std::move(source2Imm)),
                                                                               isImmediate(true), update(false) { }

        bool isUpdate() const {
            return update;
        }

        void setUpdate(bool update) {
            AdditionInstruction::update = update;
        }

        const Operands::Register &getTarget() const {
            return target;
        }

        void setTarget(const Operands::Register &target) {
            AdditionInstruction::target = target;
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
            std::string ans = "add";
            if (update)
                ans += "s";
            const Operands::Operand* source_2 = isImmediate ? static_cast<const Operands::Operand *>(&source_2_imm)
                                                            : static_cast<const Operands::Operand *>(&source_2_flex);
            ans += " " + target.toASM() + ", " + source_1.toASM() + ", " + source_2->toASM();
            return ans;
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

    class LoadSaveProto : public MachineInstruction {
    protected:
        Operands::Register target;
    };

    class LoadInstruction : public LoadSaveProto {

    };

    class SaveInstruction : public LoadSaveProto {

    };

    typedef std::vector<std::shared_ptr<MachineInstruction>> InstructionStream;
}

#endif //SYSYBACKEND_INSTRUCTION_H
