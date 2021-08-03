#include <iostream>

#include "ast.h"
#include "genIR.h"
#include "eliPhi.h"
#include "IRTranslator.h"
#include "RegisterAllocation.h"
#include "RegisterAllocationRefactor.h"

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

    //backend runner
    runner(*programIR);
    return 0;
}
