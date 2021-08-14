//
// Created by 廖治平 on 6/6/21.
//

#ifndef SYSYBACKEND_IRUTILITIES_H
#define SYSYBACKEND_IRUTILITIES_H

#include <string>

namespace IntermediateRepresentation::Utility {
    const std::string IRStmtTypeToStr[] {
        "br", "add", "mul", "div", "mod", "sub",
        "call", "return", "mov", "label", "load",
        "store", "alloca", "cmp_eq", "cmp_ne",
        "cmp_sge", "cmp_sle", "cmp_sgt", "cmp_slt",
        "glb_const", "glb_var", "glb_arr", "lsh",
        "rsh", "or", "and", "xor", "not", "phi",
        "param", "stk_load", "stk_str"
    };
    const std::string IRDataTypeToStr[] {
        "i32", "str", "t_void"
    };
    const std::string IROpTypeToStr[] {
        "ImmVal", "Var", "Null"
    };
}

#endif //SYSYBACKEND_IRUTILITIES_H
