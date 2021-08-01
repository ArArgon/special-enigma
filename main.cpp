#include <iostream>
#include "Instruction.h"
#include "IRTypes.h"
#include "IRTranslator.h"
#include "RegisterAllocationRefactor.h"

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
    is << std::move(br1);
    is << AdditionInstruction(pc, r1, FlexibleOperand(r2, imm8(10), imm5(3), true));
    for (const auto& var : is)
        std::cout << *var << std::endl;
}

void runner(const IntermediateRepresentation::IRProgram& irProgram) {
    auto &&translator = Backend::Translator::Translator<Backend::RegisterAllocation::ColourAllocatorRewrite, Backend::Translator::availableRegister>(irProgram);
    try {
        auto ins = translator.doTranslation();
        puts("");
        puts("[ASM]");
        for (const auto& var : ins)
            std::cout << *var << std::endl;
    } catch (const std::runtime_error& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        exit(-1);
    }
}

void testCase1() {
    using namespace IntermediateRepresentation;
    IRProgram program;

    Function function("Test");

    IRArray glb_arr("glb_arr_tst", 10);
    glb_arr.addData(5, 0xafff);

    auto a = IROperand(i32, "a"), b = IROperand(i32, "b"),
         c = IROperand(i32, "c"), d = IROperand(i32, "d"),
         e = IROperand(i32, "e"), f = IROperand(i32, "f"),
         glb = IROperand(i32, "glb_tst"), glb_str = IROperand(str, "glb_str");

    program.setGlobal( {
        Statement { GLB_VAR, i32, glb },
        Statement { GLB_VAR, str, glb_str, IROperand("%d %d %d\\n") }
    } );
    program.setGlobalArrays( { glb_arr } );

    auto genImm = [] (int imm) {
        return IROperand(i32, imm);
    };

    function.setParameters( {
        a, b, c, d
    } );
//
//    function << Statement { ADD, i32, a, b, a };
//    function << Statement { SUB, i32, b, c, a };
//    function << Statement { MOD, i32, d, a, b };

    function << Statement { CALL, i32, d, IROperand("printf"), glb_str, a, d, b, c };
    function << Statement { RETURN, i32, c };

    program.setFunctions( { function } );
    std::cout << program.toString() << std::endl;

    runner(program);
}

int main() {
    IntermediateRepresentation::IRProgram irProgram;
    IntermediateRepresentation::Function function("Test");
    IntermediateRepresentation::Statement statement(IntermediateRepresentation::ADD, IntermediateRepresentation::i32, IntermediateRepresentation::IROperand(IntermediateRepresentation::i32, 32));
    using namespace IntermediateRepresentation;

    auto &&translator = Backend::Translator::Translator<Backend::RegisterAllocation::ColourAllocatorRewrite, Backend::Translator::availableRegister>(irProgram);

    /*
     * mov      %a, 5
     * mov      %b, 3
     * div      %c, %a, %b
     * return   %c
     * */
    function.insert(Statement(MOV, i32, IROperand(i32, "a"), IROperand(i32, 5)));
    function << Statement { MOV, i32, IROperand(i32, "b"), IROperand(i32, 3) };
    function << Statement { DIV, i32, IROperand(i32, "c"), IROperand(i32, "a"), IROperand(i32, "b") };
    function << Statement { RETURN, i32, IROperand(i32, "c") };
    irProgram.insert(function);

    testCase1();

//    std::cout << irProgram.toString();
//    auto &&translator = Backend::Translator::Translator<Backend::RegisterAllocation::ColourAllocator, Backend::Translator::availableRegister>(irProgram);
//    try {
//        auto &&ins = translator.doTranslation();
//        puts("");
//        puts("[ASM]");
//        for (const auto& var : ins)
//            std::cout << *var << std::endl;
//    } catch (std::exception e) {
//        std::cerr << "Fatal error: " << e.what() << std::endl;
//        exit(-1);
//    }
    // testASM();
    return 0;
}
