#ifndef _GENSSA_H_

#define _GENSSA_H_

#include <stack>

#include "ast.h"
#include "IRTypes.h"
#include "symtab.h"

/*extern IntermediateRepresentation::IRProgram *ssaIR;
extern symbalTable symTab;
extern std::vector<IntermediateRepresentation::Statement> my_global;
extern std::vector<IntermediateRepresentation::Function> my_functions;
extern std::vector<IntermediateRepresentation::IRArray> my_globalArrays;*/

IntermediateRepresentation::IRProgram* genIR(AST *astRoot);
void trans_declaration(AST* a, bool isglobal);
void trans_declaration_IR(AST* a, std::string type, bool isconst, bool isglobal);
void pri_var(AST* a, std::string type, bool isconst, bool isglobal);
void pri_array(AST* a, std::string type, bool isconst, bool isglobal);
void pri_global_arr_init_list(AST* a, IntermediateRepresentation::IRArray& globalArray, std::vector<int> arrayIndex, int& position, int level);
void pri_arr_init_list(AST* a, std::string s_name, std::vector<int> arrayIndex, int& index, int level);

void trans_func_def(AST* a);
void trans_param(AST* a, std::string func_name, std::vector<int> &value);
void trans_block(AST* a);

void pri_exp_statement(AST* a);
void pri_if(AST* a);
void pri_if_else(AST* a);
void pri_while(AST *a);
IntermediateRepresentation::IROperand pri_cond(AST* a);
void pri_single_statement_block(AST* a);
void pri_no_return_func(AST* a);
IntermediateRepresentation::IROperand pri_return_func(AST* a);
IntermediateRepresentation::IROperand pri_exp(AST* a);
IntermediateRepresentation::IROperand pri_const_var_exp(AST* a, bool isglobal);
IntermediateRepresentation::IROperand pri_arr_postfix_expression(AST* a);
void pri_LVal_arr_postfix_expression(AST* LVal, AST* exp);

std::string getNewNameLocalVar();
std::string getNewLabel();
void sysy_runingtime_func_init();
#endif
