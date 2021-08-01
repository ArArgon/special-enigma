cmake_minimum_required(VERSION 3.10)
project(SysYBackend)

set(CMAKE_CXX_STANDARD 17)

add_executable(SysYBackend main.cpp IRTypes.h Instruction.h InstructionOperands.h InstructionUtilities.h IRUtilities.h Utilities.cpp Utilities.h IRTypes.cpp Flow.cpp Flow.h InstructionUtilities.cpp RegisterAllocationRefactor.h)