#include <iostream>
#include "Instruction.h"
#include "InstructionUtilities.h"
#include "IRTypes.h"

using Instruction::ARMv7_Register;
using Instruction::BranchInstruction;
using namespace Instruction;
using namespace Instruction::Operands;
using namespace Instruction::Utilities::Abbr;

void testASM() {
    std::cout << "Hello, World!" << std::endl;
    Instruction::Operands::RegisterList list;
    list.emplaceRegister(Instruction::ARMv7_Register::PC, Instruction::ARMv7_Register::R0, Instruction::ARMv7_Register::R1);
    auto add1 = Instruction::AdditionInstruction(Instruction::Operands::Register(Instruction::R1), Instruction::Operands::Register(Instruction::PC),
                                                 Instruction::Operands::FlexibleOperand(Instruction::Operands::Register(Instruction::R1), Instruction::Operands::ImmediateNumber<8>(10), Instruction::Operands::ImmediateNumber<5> (30)));

    auto br1 = BranchInstruction(BL, "__FL1");
    auto cmp1 = ComparisonInstruction(CMP, Register(R0), Register(LR));
    std::cout << PushInstruction(list) << std::endl;
    std::cout << add1 << std::endl;
    std::cout << AdditionInstruction(Register(R1), Register(R2), imm12(-50)) << std::endl;
    std::cout << br1 << std::endl;
    std::cout << cmp1 << std::endl;
    std::cout << AdditionInstruction(r0, Register(PC), Register(LR)) << std::endl;
    std::cout << BitInstruction(AND, r0, Register(PC), Register(LR), true) << std::endl;
    std::cout << LabelInstruction(Operands::Label("s")) << std::endl;
    std::cout << LabelInstruction("__FL1") << std::endl;
    std::cout << AdditionInstruction(r1, r2, r0) << std::endl;
    std::cout << BranchInstruction(B, Label(".")) << std::endl;
    std::cout << offset8(-1099) << std::endl;
    InstructionStream is;
    is << ComparisonInstruction(CMP, r0, pc) << BranchInstruction(B, Label("__next_gb"));

    is << AdditionInstruction(pc, r1, FlexibleOperand(r2, imm8(10), imm5(3), true));
    for (const auto& var : is)
        std::cout << *var << std::endl;
}

int main() {

    IntermediateRepresentation::IRProgram irProgram;
    IntermediateRepresentation::Function function("Test");
    IntermediateRepresentation::Statement statement(IntermediateRepresentation::CALL, IntermediateRepresentation::IROperand());
    return 0;
}
