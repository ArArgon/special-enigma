#include <iostream>

#include "ast.h"
#include "genIR.h"
#include "eliPhi.h"

int main(int argc, char *argv[]) {

    AST *ast = parseAST(argv[1]);

    showast(ast, 1);

    IntermediateRepresentation::IRProgram *programIR = genIR(ast);
    deleteAST(ast);

    //SSA
    //programIR = genSSA(programIR);

    std::cout << programIR->toString() << std::endl;

    //消除phi函数
    //programIR = eliminatePhi(programIR);
    return 0;
}
