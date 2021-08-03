cmake_minimum_required(VERSION 3.10)
project(SysYBackend)
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-ll")
flex_target(lexer lrlex.l "${CMAKE_CURRENT_SOURCE_DIR}/lex.yy.cpp")
bison_target(parser lrparser.y "${CMAKE_CURRENT_SOURCE_DIR}/lrparser.tab.cpp")

add_executable(SysYBackend main.cpp lrparser.tab.cpp lex.yy.cpp ast.cpp genIR.cpp IRTypes.cpp IRTypes.h Instruction.h InstructionOperands.h InstructionUtilities.h IRUtilities.h Utilities.cpp Utilities.h IRTypes.cpp Flow.cpp Flow.h InstructionUtilities.cpp RegisterAllocationRefactor.h)

add_executable(SysYBackendTest backendTest.cpp lrparser.tab.cpp lex.yy.cpp ast.cpp genIR.cpp IRTypes.cpp IRTypes.h Instruction.h InstructionOperands.h InstructionUtilities.h IRUtilities.h Utilities.cpp Utilities.h IRTypes.cpp Flow.cpp Flow.h InstructionUtilities.cpp RegisterAllocationRefactor.h)