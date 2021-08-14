
#ifndef IRTYPES_CPP_OPT_H
#define IRTYPES_CPP_OPT_H
using namespace IntermediateRepresentation;

int optimization(IRProgram *programIR);
int create_cfg(IRProgram *programIR);
int create_gen_kill(IRProgram *programIR);
int create_in_out(IRProgram *programIR);
int create_use_def(IRProgram *programIR);
int clear_use_def(IRProgram *programIR);

int constant_propagation(IRProgram *programIR);

int debug_opt(IRProgram *programIR);
int debug_cfg(IRProgram *programIR);
int debug_gen_kill(IRProgram *programIR);
int debug_in_out(IRProgram *programIR);
int debug_use_def(IRProgram *programIR);

#endif //IRTYPES_CPP_OPT_H
