//
// Created by 廖治平 on 7/30/21.
//

#include "InstructionUtilities.h"

namespace Instruction::Utilities {

    bool ASMFormatter::usingTab = true;

    bool isTooLong(int32_t val)  {
        uint32_t u_val = reinterpret_cast<int32_t>(val);
        while (!(u_val & 1))
            u_val >>= 1;
        int high;
        for (high = 31; high > 15; high--)
            if (u_val & 1 << (high))
                break;
            return high > 15;
    }

    std::string ASMFormatter::toASM() const  {
        if (isLabel)
            return ins;
        else {
            std::string ans;
            if (usingTab)
                ans += "\t\t";
            else
                ans += "        ";
            ans += ins;
            if (usingTab)
                ans += std::string("\t") + (ins.size() < 4 ? "\t" : "");
            else {
                for (int i = 8 - ins.size(); i >= 1; i--)
                    ans += " ";
            }
            return ans + opr;
        }
    }
}