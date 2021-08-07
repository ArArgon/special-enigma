//
// Created by 廖治平 on 5/22/21.
//

#ifndef SYSYBACKEND_INSTRUCTIONOPERANDS_H
#define SYSYBACKEND_INSTRUCTIONOPERANDS_H

#include <vector>
#include <string>
#include <bitset>
#include <functional>
#include <exception>
#include <utility>
#include <ostream>

namespace Instruction {
    enum ARMv7_Register {
        R0 = 0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15,
        PC = R15, LR = R14, SP = R13, IP = R12, FP = R11
    };
    const std::string regTable[] = {
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "fp", "ip", "sp", "lr", "pc"
    };
}

namespace Instruction::Operands {
    // Operand base

    typedef std::string Label;

    class Operand {

    public:
        virtual std::string toASM() const = 0;
        virtual bool isSame(const Operand& opr) const {
            return this->toASM() == opr.toASM();
        };

        friend std::ostream &operator<<(std::ostream &os, const Operand &operand) {
            os << operand.toASM();
            return os;
        }

        bool operator== (const Operand& opr2) const {
            return isSame(opr2);
        }
    };

    template<int len>
    class ImmediateNumber : public Operand {
        int32_t Value;

        inline int32_t tailor(int32_t val) {
            if (len > 0)
                return val & ((1u << len) - 1u);
            if (!len)
                return val;
            // len < 0
            return (abs(val) & 0x3ff) * (-1 + (val >= 0) * 2);

        }
    public:

        static bool isTooLong(int num) {
            if (num >= 0) {
                return num > ((1u << len) - 1);
            } else {
                return (-num) > ((1u << (len - 1)) - 1);
            }
        }

        explicit ImmediateNumber(int32_t value) : Value(tailor(value)) { }
        ImmediateNumber() : Value(0) { }

        bool operator< (const ImmediateNumber &rhs) const {
            return Value < rhs.getValue();
        }

        bool operator> (const ImmediateNumber &rhs) const {
            return rhs.getValue() < Value;
        }

        ImmediateNumber& operator= (int32_t value) {
            Value = tailor(value);
            return *this;
        }

        int32_t getValue() const {
            return Value;
        }

        void setValue(uint32_t value) {
            Value = tailor(value);
        }

        std::string toASM() const override {
            return std::string("#") + std::to_string(Value);
        }
    };

    class Register : public Operand {
        ARMv7_Register reg = R0;
    public:
        bool operator==(const Register &rhs) const {
            return reg == rhs.getReg();
        }

        bool operator!=(const Register &rhs) const {
            return !(rhs == *this);
        }

        explicit Register(ARMv7_Register reg) : reg(reg) {}
        Register() = default;

        ARMv7_Register getReg() const {
            return reg;
        }

        void setReg(ARMv7_Register reg) {
            Register::reg = reg;
        }

        std::string toASM() const {
            if (reg > ARMv7_Register::R15)
                throw std::runtime_error(std::string("Unrecognized register: ") + std::to_string(reg));
            return regTable[reg];
        }
    };

    class RegisterList : public Operand {
        std::vector<ARMv7_Register> regList;

        void emplaceRegister() { }
    public:
        template<class... Args>
        RegisterList(Args... args) {
            (regList.push_back(args), ...);
        }

        RegisterList() = default;
        RegisterList(const std::vector<Register>& regs) {
            for(auto& var : regs)
                insertRegister(var);
        }

        const std::vector<ARMv7_Register> &getRegList() const {
            return regList;
        }

        void setRegRecord(const std::vector<ARMv7_Register> &regRecord) {
            for (auto var : regRecord)
                insertRegister(var);
        }

        void insertRegister(const Register& reg) {
            insertRegister(reg.getReg());
        }

        void insertRegister(ARMv7_Register reg) {
            if (reg > ARMv7_Register::PC)
                throw std::runtime_error(std::string("Unexpected register: ") + std::to_string(reg));
            regList.push_back(reg);
        }

        template<class Cur, class... Pack>
        void emplaceRegister(const Cur& reg, Pack... args) {
            insertRegister(reg);
            emplaceRegister(args...);
        }
    public:
        std::string toASM() const override {
            std::string ans = "{ ";
            bool isBegin = true;
            for (auto reg : regList) {
                if (isBegin)
                    isBegin = false;
                else
                    ans += ", ";
                ans += regTable[reg];
            }
            return ans + " }";
        }
    };

    class LoadSaveOperand : public Operand {
        Register reg, offReg;
        int64_t offset;
        bool addrAll, isOffReg = false;

    public:
        LoadSaveOperand() = default;

        LoadSaveOperand(Register reg, int64_t offset, bool addrAll) : reg(std::move(reg)), offset(offset),
                                                                             addrAll(addrAll) { }

        LoadSaveOperand(const Register &reg, const Register &offReg, bool addrAll) : reg(reg), offReg(offReg),
                                                                                     addrAll(addrAll), isOffReg(true) { }

        LoadSaveOperand(Register reg) : reg(std::move(reg)), offset(0), addrAll(true) { }

        std::string toASM() const override {
            std::string ans = "[" + reg.toASM();
            if (addrAll) {
                if (offset && !isOffReg)
                    ans += ", #" + std::to_string(offset);
                if (isOffReg)
                    ans += ", " + offReg.toASM();
                ans += "]";
            } else {
                ans += "]";
                if (offset && !isOffReg)
                    ans += ", #" + std::to_string(offset);
                if (isOffReg)
                    ans += ", " + offReg.toASM();
            }
            return ans;
        }
    };

    class FlexibleOperand : public Operand {
        Register reg;
        ImmediateNumber<8> immediateNumber;
        ImmediateNumber<5> shift_bits;

        // 0: Register, 1: Immediate number, 2: left move, 3: right move;
        int flexType;

    public:
        FlexibleOperand() = default;
        FlexibleOperand(Register reg) : reg(std::move(reg)), flexType(0) { }

        FlexibleOperand(const ImmediateNumber<8> &immediateNumber) : immediateNumber(immediateNumber), flexType(1) { }

        FlexibleOperand(Register reg, const ImmediateNumber<8> &immediateNumber, const ImmediateNumber<5> &shiftBits,
                        bool movRight = false) : immediateNumber(immediateNumber), shift_bits(shiftBits), reg(std::move(reg)), flexType(movRight ? 3 : 2) { }

        std::string toASM() const override {
            std::string ans = "";
            switch (flexType) {
                case 0:
                    ans += reg.toASM();
                    break;
                case 1:
                    ans += immediateNumber.toASM();
                    break;
                case 2:
                    ans += reg.toASM() + " lsl " + shift_bits.toASM();
                    break;
                case 3:
                    ans += reg.toASM() + " lsr " + shift_bits.toASM();
                    break;
                default:
                    throw std::runtime_error("Unexpected flexible operand type: " + std::to_string(flexType));
            }
            return ans;
        }
    };

    typedef FlexibleOperand Operand2;
}

#endif //SYSYBACKEND_INSTRUCTIONOPERANDS_H
