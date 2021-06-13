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

    bool isTooLong(int32_t val) {
        uint32_t u_val = reinterpret_cast<int32_t>(val);
        while (!(u_val & 1))
            u_val >>= 1;
        int high;
        for (high = 31; high > 15; high--)
            if (u_val & 1 << (high))
                break;
        return high > 15;
    }

    namespace Abbr {
        typedef Operands::ImmediateNumber<8> imm8;
        typedef Operands::ImmediateNumber<12> imm12;
        typedef Operands::ImmediateNumber<16> imm16;
        typedef Operands::ImmediateNumber<32> imm32;
        typedef Operands::ImmediateNumber<5> imm5;
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
