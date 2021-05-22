//
// Created by 廖治平 on 5/22/21.
//

#ifndef SYSYBACKEND_INSTRUCTIONUTILITIES_H
#define SYSYBACKEND_INSTRUCTIONUTILITIES_H

#include "Instruction.h"

namespace Instruction::Utilities {
    /*
    * Immediate number abbr
    * */
    std::vector<MachineInstruction> translateInteger() {
        // TODO
    }
    namespace Abbr {
        typedef Operands::ImmediateNumber<8> imm8;
        typedef Operands::ImmediateNumber<12> imm12;
        typedef Operands::ImmediateNumber<16> imm16;
        typedef Operands::ImmediateNumber<32> imm32;
        typedef Operands::ImmediateNumber<0> offset;
        typedef Operands::ImmediateNumber<-8> offset8;

        /*
         * Register instances
         * */
        static const Operands::Register r0(R0), r1(R1), r2(R2), r3(R3),
                r4(R4), r5(R5), r6(R6), r7(R7),
                r8(R8), r9(R9), r10(R10), r11(R11),
                r12(R12), r13(R13), r14(R14), r15(R15),
                pc(PC), lr(LR), fp(FP), ip(IP),
                sp(SP);
    }
}

#endif //SYSYBACKEND_INSTRUCTIONUTILITIES_H
