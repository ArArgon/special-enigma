#include <iostream>
#include <fstream>

#include "ast.h"
#include "genIR.h"
#include "eliPhi.h"
#include "IRTranslator.h"
#include "RegisterAllocation.h"
#include "RegisterAllocationRefactor.h"

bool isDebug;
int optLevel;

void runner(const IntermediateRepresentation::IRProgram& irProgram, std::ostream& os) {
    auto &&translator = Backend::Translator::Translator<Backend::RegisterAllocation::ColourAllocatorRewrite, Backend::Translator::availableRegister>(irProgram);
    if (!os.good()) {
        std::cerr << "Fatal error: output stream is not prepared." << std::endl;
        exit(-1);
    }
    try {
        auto ins = translator.doTranslation();
        for (const auto& var : ins)
            os << *var << std::endl;
    } catch (const std::runtime_error& e) {
        os << "Fatal error: " << e.what() << std::endl;
        exit(-1);
    }
}

bool doesFileExist(const std::string& filePath) {
    std::ifstream fs { filePath };
    bool ans = false;
    if (fs.is_open() && fs.good())
        ans = true;
    fs.close();
    return ans;
}

int main(int argc, char *argv[]) {
    std::vector<std::string> args { argv + 1, argv + argc };

    std::string asmOut, sourceIn;
    bool outputASM = false;
    for (auto it = args.begin(); it != args.end(); it++) {
        if ((*it)[0] == '-') {
            if (*it == "-O2")
                optLevel = 2;
            if (*it == "-o")
                asmOut = *++it;
            if (*it == "-S")
                outputASM = true;
            if (*it == "-debug")
                isDebug = true;
        } else
            sourceIn = *it;
    }

    if (sourceIn.empty()) {
        std::cerr << "Fatal error: source file must be specified." << std::endl;
        exit(-1);
    }

    if (!outputASM) {
        std::cerr << "Fatal error: '-S' must present in the arguments." << std::endl;
        exit(-1);
    }

    if (asmOut.empty() && !isDebug) {
        std::cerr << "Fatal error: assembly output path must be specified." << std::endl;
        exit(-1);
    }

    if (!doesFileExist(sourceIn)) {
        std::cerr << "Fatal error: \"" << sourceIn << "\": no such file." << std::endl;
        exit(-1);
    }

    std::fstream asmOutFs;
    asmOutFs.open(asmOut, std::fstream::out);

    if ((!asmOutFs.is_open() || asmOutFs.bad()) && !isDebug) {
        std::cerr << "Fatal error: unable to open file \"" << asmOut << "\"" << std::endl;
        exit(-1);
    }

    AST *ast = parseAST(sourceIn.c_str());

    if (isDebug)
        showast(ast, 1);
    
    IntermediateRepresentation::IRProgram *programIR = genIR(ast);
    deleteAST(ast);

    //SSA
    //programIR = genSSA(programIR);

    if (isDebug)
        std::cout << programIR->toString() << std::endl;
    
    //消除phi函数
    //programIR = eliminatePhi(programIR);

    //backend runner
    if (isDebug && !asmOutFs.is_open())
        runner(*programIR, std::cout);
    else
        runner(*programIR, asmOutFs);
    return 0;
}
