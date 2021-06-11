cmake_minimum_required(VERSION 3.17)
project(SysYBackend)

set(CMAKE_CXX_STANDARD 17)

add_executable(SysYBackend main.cpp IRTypes.h Instruction.h InstructionOperands.h InstructionUtilities.h IRTranslator.h IRUtilities.h)