//
// Created by 廖治平 on 7/18/21.
//

#include "Utilities.h"

namespace Backend::Util {
    size_t StackScheme::allocate(const StackScheme::var_t &opr, size_t size) {
        if (size & 0b11) {
            // not 4-align
            return -1;
        }
        size_t pos = currentStackSize;
        inStackVariables.insert(opr);
        variablePosition[opr] = pos;
        currentStackSize += size;
        return pos;
    }

    size_t StackScheme::getVariablePosition(const StackScheme::var_t &opr) const {
        return inStackVariables.count(opr) ? variablePosition.at(opr) : -1;
    }

    bool StackScheme::isInStack(const StackScheme::var_t &opr) const {
        return inStackVariables.count(opr) > 0;
    }

    size_t StackScheme::getStackSize() const {
        return currentStackSize;
    }
}
