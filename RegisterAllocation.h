//
// Created by 廖治平 on 7/18/21.
//

#ifndef SYSYBACKEND_IRREGISTERALLOCATION_H
#define SYSYBACKEND_IRREGISTERALLOCATION_H

#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "IRTypes.h"
#include "Utilities.h"
#include "InstructionOperands.h"

namespace Backend::RegisterAllocation {
    // A virtual class for register allocation algorithms

    template<size_t registerCount>
    class RegisterAllocator {
    protected:
        IntermediateRepresentation::Function sourceFunc;
        std::map<IntermediateRepresentation::IROperand, int> allocation;
        std::set<IntermediateRepresentation::IROperand> variables;
        virtual void doFunctionScan();
    public:

        ~RegisterAllocator() = default;
        explicit RegisterAllocator(IntermediateRepresentation::Function func) : sourceFunc(std::move(func)) { };
        RegisterAllocator() = default;
        const auto& getAllocation() { return allocation; }
        const auto& getVariables() { return variables; }
    };

    template<size_t registerCount>
    class ColourAllocator : public RegisterAllocator<registerCount> {

        using baseType = RegisterAllocator<registerCount>;
        using BasicBlock = Util::BasicBlock;
        using bb_ptr_t = std::shared_ptr<BasicBlock>;
        std::vector<bb_ptr_t> basicBlocks;
        Backend::Util::Graph<bb_ptr_t> cfg;

        void analyzeBasicBlocks();

        void analyzeLiveness();

        void coalesce();
    public:
        explicit ColourAllocator(const IntermediateRepresentation::Function& func) : baseType::RegisterAllocator(func) {
            analyzeBasicBlocks();
            analyzeLiveness();
        }
    };

}

#endif //SYSYBACKEND_IRREGISTERALLOCATION_H
