//
// Created by 廖治平 on 7/18/21.
//

#include "IRTypes.h"

#include <utility>

namespace IntermediateRepresentation {
    size_t NamingUtil::counter = 0;

    void IRArray::addData(size_t position, int value) {
        if (position >= arrSize)
            return;
        if (value == defaultValue && data.count(position))
            data.erase(position);
        else
            data[position] = value;
    }

    IRArray::IRArray(std::string arrayName, size_t arrSize) : arrayName(std::move(arrayName)), arrSize(arrSize), defaultValue(0) { }

    IRArray::IRArray(std::string arrayName, size_t arrSize, int defaultValue) : arrayName(std::move(arrayName)),
                                                                                       arrSize(arrSize),
                                                                                       defaultValue(defaultValue) {}
}