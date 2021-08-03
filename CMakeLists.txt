cmake_minimum_required(VERSION 3.10)
project(SysYBackend)
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_FLAGS "-ll")
flex_target(lexer lrlex.l "${CMAKE_CURRENT_SOURCE_DIR}/lex.yy.cpp")
bison_target(parser lrparser.y "${CMAKE_CURRENT_SOURCE_DIR}/lrparser.tab.cpp")

add_executable(SysYCompiler compiler.cpp ${BISON_parser_OUTPUTS} ${FLEX_lexer_OUTPUTS} ast.cpp genIR.cpp Utilities.cpp Utilities.h IRTypes.cpp Flow.cpp InstructionUtilities.cpp)

add_executable(SysYFrontendTest frontendTest.cpp ${BISON_parser_OUTPUTS} ${FLEX_lexer_OUTPUTS} ast.cpp genIR.cpp IRTypes.cpp)

add_executable(SysYBackendTest backendTest.cpp ${BISON_parser_OUTPUTS} ${FLEX_lexer_OUTPUTS} ast.cpp genIR.cpp Utilities.cpp IRTypes.cpp Flow.cpp InstructionUtilities.cpp IRTranslatorSimple.h)