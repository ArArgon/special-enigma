#include <iostream>
#include <string>
#include <vector>
#include <stack>

#include "genIR.h"
#include "ast.h"

symbalTable *symTab;
int symTabLevel;
std::vector<IntermediateRepresentation::Statement> *my_global;
std::vector<IntermediateRepresentation::Function> *my_functions;
std::vector<IntermediateRepresentation::IRArray> *my_globalArrays;
IntermediateRepresentation::Function* my_function;
int localVarNum;
int labelNum = 0;
std::stack<whileLable> whileLables;


IntermediateRepresentation::IRProgram* genIR(AST *astRoot)
{
    IntermediateRepresentation::IRProgram *ssaIR = new IntermediateRepresentation::IRProgram;
    symTab = new symbalTable;
    my_global = new std::vector<IntermediateRepresentation::Statement>;
    my_functions = new std::vector<IntermediateRepresentation::Function>;
    my_globalArrays = new std::vector<IntermediateRepresentation::IRArray>;
    
    std::stack<AST*> astStack;
    AST *temp = astRoot;
    sysy_runingtime_func_init();
    
    while(temp){    //语法树节点入栈
        astStack.push(temp);
        temp=temp->left;
    }

    while(!astStack.empty()){
        temp = astStack.top()->right;
        astStack.pop();

        if(temp->name=="declaration"){
            trans_declaration(temp, true);
        }
        else if(temp->name=="function_definition"){
            trans_func_def(temp);
        }
    }
    ssaIR->setGlobal(*my_global);
    ssaIR->setGlobalArrays(*my_globalArrays);
    ssaIR->setFunctions(*my_functions);

    delete symTab;
    delete my_global;
    delete my_globalArrays;
    delete my_functions;
    
    return ssaIR;
}

void trans_declaration(AST* a, bool isglobal)
{
    AST* temp;
    std::string type;
    bool isconst = 0;
    std::stack<AST*> declStack;

    temp = a->left;
    type = temp->name;
    temp = temp->left;
    if(temp){
        if(temp->name == "CONST")
            isconst = 1;
    }

    temp = a->right;
    while(temp){
        declStack.push(temp);
        temp=temp->left;
    }
    while(!declStack.empty()){
        temp = declStack.top()->right;
        declStack.pop();
        trans_declaration_IR(temp, type, isconst, isglobal);
    }
}

void trans_declaration_IR(AST* a, std::string type, bool isconst, bool isglobal)
{
    if(a->name=="ASSIGN")
    {
        AST *temp = a->left;
        if(temp->name=="IDENTIFIER"){
            pri_var(a, type, isconst, isglobal);
        }
        else{
            pri_array(a, type, isconst, isglobal);
        }
    }
    else{
        if(a->name=="IDENTIFIER")
        {
            pri_var(a, type, isconst, isglobal);
        }
        else{
            pri_array(a, type, isconst, isglobal);
        }
    }
}

void pri_var(AST* a, std::string type, bool isconst, bool isglobal)
{
    
    if(isglobal)
    {
        if(a->name=="ASSIGN")
        {
            AST* temp = a->left;
            std::string name = temp->content;
            temp = a->right;
            IntermediateRepresentation::IROperand ops_value = pri_const_var_exp(temp, isglobal);
            int value;
            if(ops_value.getIrOpType() == IntermediateRepresentation::ImmVal)
                value = ops_value.getValue();
            else
                std::cout << "error at global var init" << std::endl;
            //std::cout << name << " " << value << std::endl; 
            IntermediateRepresentation::IROperand my_ops0(IntermediateRepresentation::i32, name);
            IntermediateRepresentation::IROperand my_ops1(IntermediateRepresentation::i32, value);
            IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::GLB_VAR, IntermediateRepresentation::i32, my_ops0, my_ops1);
            my_global->push_back(tempVar);
            //add to sym tab
            symbalTableMember symTabM;
            symTabM.init(name, name, symbalTableMember::INT, 0);
            symTabM.initialized = 1;
            symTabM.value.push_back(value);
            symTab->addGlobalVar(symTabM);
        }
        else
        {
            std::string name = a->content;

            IntermediateRepresentation::IROperand my_ops0(IntermediateRepresentation::i32, name);
            IntermediateRepresentation::IROperand my_ops1(IntermediateRepresentation::i32, 0);
            IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::GLB_VAR, IntermediateRepresentation::i32, my_ops0, my_ops1);
            my_global->push_back(tempVar);

            //add to sym tab
            symbalTableMember symTabM;
            symTabM.init(name, name, symbalTableMember::INT, 0);
            symTab->addGlobalVar(symTabM);
        }
    }
    else
    {
        if(a->name=="ASSIGN")
        {
            AST* temp = a->left;
            std::string name = temp->content;
            std::string s_name = name+"_"+getNewNameLocalVar();
            temp = a->right;
            IntermediateRepresentation::IROperand my_ops0(IntermediateRepresentation::i32, s_name);
            IntermediateRepresentation::IROperand my_ops1 = pri_exp(temp);
            IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MOV, IntermediateRepresentation::i32, my_ops0, my_ops1);
            my_function->insertStatement(tempVar);
            symbalTableMember symTabM;
            symTabM.init(s_name, name, symbalTableMember::INT, symTabLevel);
            if(my_ops1.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                int value = my_ops1.getValue();
                symTabM.initialized = 1;
                symTabM.value.push_back(value);
                //std::cout<< value<< std::endl;
            }
            symTab->addLocalVar(symTabM);
        }
        else
        {
            std::string name = a->content;
            /*
            IntermediateRepresentation::IROperand my_ops0(IntermediateRepresentation::i32, s_name);
            IntermediateRepresentation::IROperand my_ops1(IntermediateRepresentation::i32, 0);
            IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MOV, IntermediateRepresentation::i32, my_ops0, my_ops1);
            my_function->insertStatement(tempVar);
            */
            symbalTableMember symTabM;
            symTabM.init(name+"_"+getNewNameLocalVar(), name, symbalTableMember::INT, symTabLevel);
            symTab->addLocalVar(symTabM);

        }
    }
}

void pri_array(AST* a, std::string type, bool isconst, bool isglobal)
{
    if(isglobal)
    {
        if(a->name=="ASSIGN")
        {
            AST* temp = a->left;
            std::stack<AST*> arr_list_stack;
            std::vector<int> arrayIndex;
            int num = -1;
            while(temp)
            {
                arr_list_stack.push(temp);
                temp = temp->left;
                num++;
            }

            temp = arr_list_stack.top();
            arr_list_stack.pop();
            std::string name = temp->content;
            arrayIndex.push_back(num);

            int arrSize = 1;
            while(!arr_list_stack.empty())
            {
                temp = arr_list_stack.top()->right;
                arr_list_stack.pop();
                if(temp->name == "CONSTANT")
                {
                    arrayIndex.push_back(temp->value);
                    arrSize *= temp->value;
                }
                else if(temp->name == "IDENTIFIER")
                {
                    int i = symTab->findInGlobal(temp->content, symbalTableMember::INT);
                    symbalTableMember symTabTemp = symTab->getGlobalVar(i);
                    int temp_value = symTabTemp.value.back();
                    //std:: cout << "temp_value :" << temp_value << std::endl;
                    arrayIndex.push_back(temp_value);
                    arrSize *= temp_value;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_Imm = pri_const_var_exp(temp, isglobal);
                    if(ops_Imm.getIrOpType() == IntermediateRepresentation::ImmVal)
                    {
                        int temp_value = ops_Imm.getValue();
                        arrayIndex.push_back(temp_value);
                        arrSize *= temp_value;

                    }
                    else
                    {
                        std::cout << "error at global array index type" << std::endl;
                        exit(-1);
                    }
                }
            }
            // arrSize *= 4; IRArry arrSize is elements

            IntermediateRepresentation::IRArray my_globalArray(name, arrSize);
            
            symbalTableMember symTabM;
            symTabM.init(name, name, symbalTableMember::ARRAY, 0);
            symTabM.arrayIndex = arrayIndex;
            symTabM.initialized = 1;

            //std::stack<AST*>().swap(arr_list_stack);
            temp = a->right;
            if(temp)
            {
                int position = 0;
                pri_global_arr_init_list(temp, my_globalArray, arrayIndex, position, 1);
            }
            my_globalArrays->push_back(my_globalArray);
            symTab->addGlobalVar(symTabM);
        }
        else
        {
            AST* temp = a;
            std::stack<AST*> arr_list_stack;
            std::vector<int> arrayIndex;
            int num = -1;
            while(temp)
            {
                arr_list_stack.push(temp);
                temp = temp->left;
                num++;
            }

            temp = arr_list_stack.top();
            arr_list_stack.pop();
            std::string name = temp->content;
            arrayIndex.push_back(num);

            int arrSize = 1;
            while(!arr_list_stack.empty())
            {
                temp = arr_list_stack.top()->right;
                arr_list_stack.pop();
                if(temp->name == "CONSTANT")
                {
                    arrayIndex.push_back(temp->value);
                    arrSize *= temp->value;
                }
                else if(temp->name == "IDENTIFIER")
                {
                    int i = symTab->findInGlobal(temp->content, symbalTableMember::INT);
                    symbalTableMember symTabTemp = symTab->getGlobalVar(i);
                    int temp_value = symTabTemp.value.back();
                    //std:: cout << "temp_value :" << temp_value << std::endl;
                    arrayIndex.push_back(temp_value);
                    arrSize *= temp_value;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_Imm = pri_const_var_exp(temp, isglobal);
                    if(ops_Imm.getIrOpType() == IntermediateRepresentation::ImmVal)
                    {
                        int temp_value = ops_Imm.getValue();
                        arrayIndex.push_back(temp_value);
                        arrSize *= temp_value;

                    }
                    else
                    {
                        std::cout << "error at global array index type" << std::endl;
                        exit(-1);
                    }
                }
            }
            //arrSize *= 4;
            
            IntermediateRepresentation::IRArray my_globalArray(name, arrSize);
            my_globalArrays->push_back(my_globalArray);

            symbalTableMember symTabM;
            symTabM.init(name, name, symbalTableMember::ARRAY, 0);
            symTabM.arrayIndex = arrayIndex;
            symTab->addGlobalVar(symTabM);
        }
    }
    else
    {
        if(a->name=="ASSIGN")
        {
            AST* temp = a->left;
            std::stack<AST*> arr_list_stack;
            std::vector<int> arrayIndex;
            int num = -1;
            while(temp)
            {
                arr_list_stack.push(temp);
                temp = temp->left;
                num++;
            }

            temp = arr_list_stack.top();
            arr_list_stack.pop();
            std::string name = temp->content;
            arrayIndex.push_back(num);

            int arrSize = 1;
            while(!arr_list_stack.empty())
            {
                temp = arr_list_stack.top()->right;
                arr_list_stack.pop();
                if(temp->name == "CONSTANT")
                {
                    arrayIndex.push_back(temp->value);
                    arrSize *= temp->value;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_Imm = pri_const_var_exp(temp, isglobal);
                    if(ops_Imm.getIrOpType() == IntermediateRepresentation::ImmVal)
                    {
                        int temp_value = ops_Imm.getValue();
                        arrayIndex.push_back(temp_value);
                        arrSize *= temp_value;
                    }
                    else
                    {
                        std::cout << "error at local array index type" << std::endl;
                        exit(-1);
                    }
                }
            }
            arrSize *= 4; //here local arrSize is bytes

            std::string s_name = name+"_"+getNewNameLocalVar();
            IntermediateRepresentation::IROperand my_ops0(IntermediateRepresentation::i32, s_name, true);
            IntermediateRepresentation::IROperand my_ops1(IntermediateRepresentation::i32, arrSize);
            IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::ALLOCA, IntermediateRepresentation::i32, my_ops0, my_ops1);
            my_function->insertStatement(tempVar);
            IntermediateRepresentation::IROperand my_dest;
            IntermediateRepresentation::IROperand ops_num0(IntermediateRepresentation::i32, 0);
            IntermediateRepresentation::IROperand ops_memset("memset");
            IntermediateRepresentation::Statement tempVar1(IntermediateRepresentation::CALL, IntermediateRepresentation::t_void, my_dest, ops_memset, my_ops0, ops_num0, my_ops1);
            my_function->insertStatement(tempVar1);

            symbalTableMember symTabM;
            symTabM.init(s_name, name, symbalTableMember::ARRAY, symTabLevel);
            symTabM.arrayIndex = arrayIndex;
            symTab->addLocalVar(symTabM);

            temp = a->right;
            //do something
            if(temp)
            {
                int index = 0;
                pri_arr_init_list(temp, s_name, arrayIndex, index, 1);
                
            }
        }
        else
        {
            AST* temp = a;
            std::stack<AST*> arr_list_stack;
            std::vector<int> arrayIndex;
            int num = -1;
            while(temp)
            {
                arr_list_stack.push(temp);
                temp = temp->left;
                num++;
            }

            temp = arr_list_stack.top();
            arr_list_stack.pop();
            std::string name = temp->content;
            arrayIndex.push_back(num);

            int arrSize = 1;
            while(!arr_list_stack.empty())
            {
                temp = arr_list_stack.top()->right;
                arr_list_stack.pop();
                if(temp->name == "CONSTANT")
                {
                    arrayIndex.push_back(temp->value);
                    arrSize *= temp->value;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_Imm = pri_const_var_exp(temp, isglobal);
                    if(ops_Imm.getIrOpType() == IntermediateRepresentation::ImmVal)
                    {
                        int temp_value = ops_Imm.getValue();
                        arrayIndex.push_back(temp_value);
                        arrSize *= temp_value;
                    }
                    else
                    {
                        std::cout << "error at local array index type" << std::endl;
                        exit(-1);
                    }
                }
            }
            arrSize *= 4;

            std::string s_name = name+"_"+getNewNameLocalVar();
            IntermediateRepresentation::IROperand my_ops0(IntermediateRepresentation::i32, s_name, true);
            IntermediateRepresentation::IROperand my_ops1(IntermediateRepresentation::i32, arrSize);
            IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::ALLOCA, IntermediateRepresentation::i32, my_ops0, my_ops1);
            my_function->insertStatement(tempVar);
            /*
            IntermediateRepresentation::IROperand my_dest;
            IntermediateRepresentation::IROperand ops_num0(IntermediateRepresentation::i32, 0);
            IntermediateRepresentation::IROperand ops_memset("memset");
            IntermediateRepresentation::Statement tempVar1(IntermediateRepresentation::CALL, IntermediateRepresentation::t_void, my_dest, ops_memset, my_ops0, ops_num0, my_ops1);
            my_function->insertStatement(tempVar1);
            */

            symbalTableMember symTabM;
            symTabM.init(s_name, name, symbalTableMember::ARRAY, symTabLevel);
            symTabM.arrayIndex = arrayIndex;
            symTab->addLocalVar(symTabM);

        }
    }
}

void pri_global_arr_init_list(AST* a, IntermediateRepresentation::IRArray& globalArray, std::vector<int> arrayIndex, int& position, int level)
{
    AST* temp = a;
    int num = arrayIndex[0];
    int onesize = 1;
    int initializer_list_num = 0;
    if(num >= level)
    {
        for(int i = level; i <= num; i++)
        {
            onesize *= arrayIndex[i];
        }
    }

    std::stack<AST*> RVal_arr_list;
    while(temp)
    {
        RVal_arr_list.push(temp);
        temp = temp->left;
        initializer_list_num++;
    }
    while(!RVal_arr_list.empty())
    {
        temp = RVal_arr_list.top()->right;
        RVal_arr_list.pop();

        if(temp)
        {
            if(temp->name == "initializer_list")
            {
                pri_global_arr_init_list(temp, globalArray, arrayIndex, position, level+1);
            }
            else
            {
                IntermediateRepresentation::IROperand ops_src = pri_exp(temp);
                if(ops_src.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    globalArray.addData(position, ops_src.getValue());
                    position++;
                    
                }
                else
                    std::cout << "error at pri_global_arr_init_list" << std::endl;
            }
        }
        else // {{}}
        {
            position += (onesize/arrayIndex[level]);
        }
    }
    if(initializer_list_num < onesize)
    {
        position = position + onesize - initializer_list_num;
    } 
    else if(initializer_list_num > onesize)
    {
        std::cout << "error at pri_arr_init_list" << std::endl;
    }
}

void pri_arr_init_list(AST* a, std::string s_name, std::vector<int> arrayIndex, int& index, int level)
{
    AST* temp = a;
    int num = arrayIndex[0];
    int onesize = 1;
    int initializer_list_num = 0;
    if(num >= level)
    {
        for(int i = level; i <= num; i++)
        {
            onesize *= arrayIndex[i];
        }
    }

    std::stack<AST*> RVal_arr_list;

    while(temp)
    {
        RVal_arr_list.push(temp);
        temp = temp->left;
        initializer_list_num++;
    }
    while(!RVal_arr_list.empty())
    {
        temp = RVal_arr_list.top()->right;
        RVal_arr_list.pop();

        if(temp)
        {
            if(temp->name == "initializer_list")
            {
                pri_arr_init_list(temp, s_name, arrayIndex, index, level+1);
            }
            else
            {
                IntermediateRepresentation::IROperand ops_src = pri_exp(temp);
                IntermediateRepresentation::IROperand my_ops0(IntermediateRepresentation::i32, s_name, true);
                IntermediateRepresentation::IROperand ops_off(IntermediateRepresentation::i32, index*4);
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::STORE, IntermediateRepresentation::i32, ops_src, my_ops0, ops_off);
                my_function->insertStatement(tempVar);
                index++;
            }
        }
        else // {{}}
        {
            index += (onesize/arrayIndex[level]);
        }
    }

    if(initializer_list_num < onesize)
    {
        index = index + onesize - initializer_list_num;
    } 
    else if(initializer_list_num > onesize)
    {
        std::cout << "error at pri_arr_init_list" << std::endl;
    }
}

void trans_func_def(AST* a)
{
    my_function = new IntermediateRepresentation::Function;
    localVarNum = 1;
    symTabLevel = 0;

    AST* temp = a->left;
    std::string type = temp->name; //INT or VOID
    if(type == "INT")
        my_function->setReturnType(IntermediateRepresentation::i32);
    else if(type == "VOID")
        my_function->setReturnType(IntermediateRepresentation::t_void);

    temp = temp->right;
    std::string name = temp->left->content;
    my_function->setFunName(name);

    temp = temp->right;
    std::vector<int> value; 
    trans_param(temp, name, value);

    symbalTableMember symTabM;
    symTabM.init(name, name, symbalTableMember::FUNC, 0);
    symTabM.value = value;
    symTab->addFunc(symTabM);

    temp = a->right;
    //std::cout << temp->name << std::endl;
    trans_block(temp);

    my_functions->push_back(*my_function);
    delete my_function;
    symTab->clearLocalVar();

}

void trans_param(AST* a, std::string func_name, std::vector<int> &value)
{
    if(a)
    {
        std::stack<AST*> paramList;
        
        AST* temp = a;
        while(temp)
        {
            paramList.push(temp);
            temp = temp->left;
        }
        while(!paramList.empty())
        {
            temp = paramList.top()->right;
            paramList.pop();
            temp = temp->right;
            if(temp->name == "IDENTIFIER") //var param
            {
                value.push_back(1);
                std::string name = temp->content;
                std::string s_name = name+"_"+getNewNameLocalVar();

                IntermediateRepresentation::IROperand param(IntermediateRepresentation::i32, s_name);
                my_function->insertParam(param);
                
                symbalTableMember symTabM;
                symTabM.init(s_name, name, symbalTableMember::INT, 1);
                symTab->addLocalVar(symTabM);

            }
            else //arr param ptr
            {
                value.push_back(0);
                std::stack<AST*> param_arr_list;
                std::vector<int> arrayIndex;
                int num = -1;
                while(temp)
                {
                    param_arr_list.push(temp);
                    temp = temp->left;
                    num++;
                }

                temp = param_arr_list.top();
                param_arr_list.pop();
                std::string name = temp->content;
                arrayIndex.push_back(num);

                while(!param_arr_list.empty())
                {
                    temp = param_arr_list.top()->right;
                    param_arr_list.pop();
                    if(temp)
                    {
                        if(temp->name == "CONSTANT")
                        {
                            arrayIndex.push_back(temp->value);
                        }
                        else
                        {
                            IntermediateRepresentation::IROperand ops_Imm = pri_const_var_exp(temp, 1);
                            if(ops_Imm.getIrOpType() == IntermediateRepresentation::ImmVal)
                            {
                                int temp_value = ops_Imm.getValue();
                                arrayIndex.push_back(temp_value);
                            }
                            else
                            {
                                std::cout<<"not find param array index id"<<std::endl;
                            }
                        }
                    }
                    else
                    {
                        arrayIndex.push_back(0);
                    }
                }
                std::string s_name = name+"_"+getNewNameLocalVar();
                IntermediateRepresentation::IROperand param(IntermediateRepresentation::i32, s_name, true);
                my_function->insertParam(param);
                symbalTableMember symTabM;
                symTabM.init(s_name, name, symbalTableMember::ARRAY, 1);
                symTabM.arrayIndex = arrayIndex;
                symTab->addLocalVar(symTabM);
            }
        }

    }
}

void trans_block(AST* a)
{
    symTabLevel++;
    std::stack<AST*> blockitem_list;
    AST* temp = a;
    while(temp)
    {
        blockitem_list.push(temp);
        temp = temp->left;
    }
    while(!blockitem_list.empty())
    {
        temp = blockitem_list.top()->right;
        blockitem_list.pop();
        //do something
          //declaration
          //expression_statement
          //if_statement
          //while_statement
          //CONTINUE
          //BREAK
          //RETURN
        if(temp)
        {
            if(temp->name == "declaration")
            {
                trans_declaration(temp, false);
            }
            else if(temp->name == "expression_statement")
            {
                pri_exp_statement(temp);
            }
            else if(temp->name == "if_statement")
            {
                if(temp->right)
                {
                    if(temp->right->name == "ELSE")
                    {
                        pri_if_else(temp);
                    }
                    else
                    {
                        pri_if(temp);
                    }
                }
                else
                {
                    pri_if(temp);
                }
                
            }
            else if(temp->name == "while_statement")
            {
                pri_while(temp);
            }
            else if(temp->name == "CONTINUE")
            {
                IntermediateRepresentation::IROperand ops_lab_cond(whileLables.top().cond);
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_lab_cond);
                my_function->insertStatement(tempVar);

            }
            else if(temp->name == "BREAK")
            {
                IntermediateRepresentation::IROperand ops_lab_after(whileLables.top().after);
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_lab_after);
                my_function->insertStatement(tempVar);
            }
            else if(temp->name == "RETURN") //return i32, no return ptr
            {
                temp = temp->right;
                if(temp)
                {
                    IntermediateRepresentation::IROperand ops_re = pri_exp(temp);
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::RETURN, IntermediateRepresentation::i32, ops_re);
                    my_function->insertStatement(tempVar);
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_re;
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::RETURN, IntermediateRepresentation::t_void, ops_re);
                    my_function->insertStatement(tempVar);
                }
            }//blockitem_list
            else if(temp->name == "blockitem_list")
            {
                trans_block(temp);
            }
            else
            {
                std::cout<< "error in blockitem_list" << std::endl;
                exit(-1);
            }
        }
    }
    /*
    std::cout << "*****" << std::endl;
    std::cout << symTabLevel << std::endl;
    std::cout << "**" << std::endl;
    */
    symTab->deleteLocalLevel(symTabLevel);
    symTabLevel--;
}

void pri_exp_statement(AST* a)
{
    AST* temp = a->right;
    if(temp->name == "ASSIGN")
    {
        AST* temp1 = temp->left;
        if(temp1->name == "IDENTIFIER")
        {
            std::string s_name = symTab->find_name(temp1->content, symbalTableMember::INT);
            IntermediateRepresentation::IROperand ops_LVal(IntermediateRepresentation::i32, s_name);
            temp1 = temp->right;
            IntermediateRepresentation::IROperand ops_src = pri_exp(temp1);
            IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MOV, IntermediateRepresentation::i32, ops_LVal, ops_src);
            my_function->insertStatement(tempVar);
        }
        else if(temp1->name == "arr_postfix_expression")
        {
            pri_LVal_arr_postfix_expression(temp1, temp->right);
        }
        else
        {
            std::cout << "error at LVal" << std::endl;
            exit(-1);
        }
    }
    else if(temp->name == "func_postfix_expression")
    {
        pri_no_return_func(temp);
    }
    else
    {
        // example: 
        //     k+1;
        pri_exp(temp);
    }
}

void pri_if(AST* a)
{
    std::string label_cond = getNewLabel();
    std::string label_stmt = getNewLabel();
    std::string label_after = getNewLabel();

    IntermediateRepresentation::IROperand ops_lab_cond(label_cond);
    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_lab_cond);
    my_function->insertStatement(tempVar);

    IntermediateRepresentation::Statement tempVar1(IntermediateRepresentation::LABEL, IntermediateRepresentation::t_void, ops_lab_cond);
    my_function->insertStatement(tempVar1);
    IntermediateRepresentation::IROperand ops_cond = pri_cond(a->left);
    IntermediateRepresentation::IROperand ops_lab_stmt(label_stmt);
    IntermediateRepresentation::IROperand ops_lab_after(label_after);
    IntermediateRepresentation::Statement tempVar2(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_cond, ops_lab_stmt, ops_lab_after);
    my_function->insertStatement(tempVar2);

    IntermediateRepresentation::Statement tempVar3(IntermediateRepresentation::LABEL, IntermediateRepresentation::t_void, ops_lab_stmt);
    my_function->insertStatement(tempVar3);
    //do something
    AST* temp = a->right;
    if(temp)
    {
        pri_single_statement_block(temp);
    }
    //do end
    IntermediateRepresentation::Statement tempVar5(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_lab_after);
    my_function->insertStatement(tempVar5);

    IntermediateRepresentation::Statement tempVar4(IntermediateRepresentation::LABEL, IntermediateRepresentation::t_void, ops_lab_after);
    my_function->insertStatement(tempVar4);
}

void pri_if_else(AST* a)
{
    std::string label_cond = getNewLabel();
    std::string label_stmt1 = getNewLabel();
    std::string label_stmt2 = getNewLabel();
    std::string label_after = getNewLabel();
    IntermediateRepresentation::IROperand ops_lab_after(label_after);

    IntermediateRepresentation::IROperand ops_lab_cond(label_cond);
    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_lab_cond);
    my_function->insertStatement(tempVar);

    IntermediateRepresentation::Statement tempVar1(IntermediateRepresentation::LABEL, IntermediateRepresentation::t_void, ops_lab_cond);
    my_function->insertStatement(tempVar1);
    IntermediateRepresentation::IROperand ops_cond = pri_cond(a->left);
    IntermediateRepresentation::IROperand ops_lab_stmt1(label_stmt1);
    IntermediateRepresentation::IROperand ops_lab_stmt2(label_stmt2);
    IntermediateRepresentation::Statement tempVar2(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_cond, ops_lab_stmt1, ops_lab_stmt2);
    my_function->insertStatement(tempVar2);

    AST* temp = a->right->left;
    IntermediateRepresentation::Statement tempVar3(IntermediateRepresentation::LABEL, IntermediateRepresentation::t_void, ops_lab_stmt1);
    my_function->insertStatement(tempVar3);
    //do something
    if(temp)
    {
        pri_single_statement_block(temp);
    }
    //do end
    IntermediateRepresentation::Statement tempVar5(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_lab_after);
    my_function->insertStatement(tempVar5);

    temp = a->right->right;
    IntermediateRepresentation::Statement tempVar6(IntermediateRepresentation::LABEL, IntermediateRepresentation::t_void, ops_lab_stmt2);
    my_function->insertStatement(tempVar6);
    //do something
    if(temp)
    {
        pri_single_statement_block(temp);
    }
    //do end
    IntermediateRepresentation::Statement tempVar7(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_lab_after);
    my_function->insertStatement(tempVar7);

    IntermediateRepresentation::Statement tempVar8(IntermediateRepresentation::LABEL, IntermediateRepresentation::t_void, ops_lab_after);
    my_function->insertStatement(tempVar8);

}

void pri_while(AST *a)
{
    whileLable whiLab;
    whiLab.cond = getNewLabel();
    whiLab.stmt = getNewLabel();
    whiLab.after = getNewLabel();
    whileLables.push(whiLab);

    IntermediateRepresentation::IROperand ops_lab_cond(whileLables.top().cond);
    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_lab_cond);
    my_function->insertStatement(tempVar);

    IntermediateRepresentation::Statement tempVar1(IntermediateRepresentation::LABEL, IntermediateRepresentation::t_void, ops_lab_cond);
    my_function->insertStatement(tempVar1);
    IntermediateRepresentation::IROperand ops_cond = pri_cond(a->left);
    IntermediateRepresentation::IROperand ops_lab_stmt(whileLables.top().stmt);
    IntermediateRepresentation::IROperand ops_lab_after(whileLables.top().after);
    IntermediateRepresentation::Statement tempVar2(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_cond, ops_lab_stmt, ops_lab_after);
    my_function->insertStatement(tempVar2);

    IntermediateRepresentation::Statement tempVar3(IntermediateRepresentation::LABEL, IntermediateRepresentation::t_void, ops_lab_stmt);
    my_function->insertStatement(tempVar3);
    //do something
    AST* temp = a->right;
    if(temp)
    {
        pri_single_statement_block(temp);
    }
    //do end
    IntermediateRepresentation::Statement tempVar5(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_lab_cond);
    my_function->insertStatement(tempVar5);

    IntermediateRepresentation::Statement tempVar4(IntermediateRepresentation::LABEL, IntermediateRepresentation::t_void, ops_lab_after);
    my_function->insertStatement(tempVar4);

    whileLables.pop();

}

void pri_single_statement_block(AST* a)
{
    AST* temp = a;
    if(temp->name == "blockitem_list")
    {
        trans_block(temp);
    }
    else if(temp->name == "expression_statement")
    {
        pri_exp_statement(temp);
    }
    else if(temp->name == "if_statement")
    {
        if(temp->right->name == "ELSE")
        {
            pri_if_else(temp);
        }
        else
        {
            pri_if(temp);
        }
    }
    else if(temp->name == "while_statement")
    {
        pri_while(temp);
    }
    else if(temp->name == "CONTINUE")
    {
        IntermediateRepresentation::IROperand ops_lab_cond(whileLables.top().cond);
        IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_lab_cond);
        my_function->insertStatement(tempVar);

    }
    else if(temp->name == "BREAK")
    {
        IntermediateRepresentation::IROperand ops_lab_after(whileLables.top().after);
        IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_lab_after);
        my_function->insertStatement(tempVar);

    }
    else if(temp->name == "RETURN") //return i32, no return ptr
    {
        temp = temp->right;
        if(temp)
        {
            IntermediateRepresentation::IROperand ops_re = pri_exp(temp);
             IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::RETURN, IntermediateRepresentation::i32, ops_re);
            my_function->insertStatement(tempVar);
        }
        else
        {
            IntermediateRepresentation::IROperand ops_re;
            IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::RETURN, IntermediateRepresentation::t_void, ops_re);
            my_function->insertStatement(tempVar);
        }
    }
    else
    {
        std::cout<< "error in statement" << std::endl;
        exit(-1);
    }
}

IntermediateRepresentation::IROperand pri_cond(AST* a)
{
    //OR_OP || ;AND_OP && ;EQ_OP == NE_OP != ;LT < GT > LE_OP <= GE_OP >= ;NOT ! ADD SUB * / %
    std::string cond_operator = a->name;
    IntermediateRepresentation::IROperand ops_l;
    IntermediateRepresentation::IROperand ops_r;

    if(cond_operator == "IDENTIFIER")
    {
        std::string s_name = symTab->find_name(a->content, symbalTableMember::INT);
        IntermediateRepresentation::IROperand ops_var(IntermediateRepresentation::i32, s_name);
        return ops_var;

    }
    else if(cond_operator == "CONSTANT")
    {
        IntermediateRepresentation::IROperand ops_Imm(IntermediateRepresentation::i32, a->value);
        return ops_Imm;
    }
    else if(cond_operator == "arr_postfix_expression")
    {
        return pri_arr_postfix_expression(a);
    }
    else if(cond_operator == "func_postfix_expression")
    {
        return pri_return_func(a);
    }
    else if(cond_operator == "AND_OP") //fix short_circuit
    {
        ops_l = pri_cond(a->left);

        if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal && !ops_l.getValue())
        {
            IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, 0);
            return ops_dest;
        }
        else
        {
            std::string label_x = getNewLabel();
            IntermediateRepresentation::IROperand ops_lab_x(label_x);
            std::string label_y = getNewLabel();
            IntermediateRepresentation::IROperand ops_lab_y(label_y);
            std::string label_z = getNewLabel();
            IntermediateRepresentation::IROperand ops_lab_z(label_z);

            IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_l, ops_lab_x, ops_lab_y);
            my_function->insertStatement(tempVar);

            IntermediateRepresentation::Statement tempVar1(IntermediateRepresentation::LABEL, IntermediateRepresentation::t_void, ops_lab_x);
            my_function->insertStatement(tempVar1);
            ops_r = pri_cond(a->right);
            IntermediateRepresentation::IROperand ops_dest;
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                if(ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    int value_l = ops_l.getValue();
                    int value_r = ops_r.getValue();
                    IntermediateRepresentation::IROperand ops_dest_t(IntermediateRepresentation::i32, value_l&&value_r);
                    ops_dest = ops_dest_t;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_dest_t(IntermediateRepresentation::i32, getNewNameLocalVar());
                    IntermediateRepresentation::Statement tempVar0(IntermediateRepresentation::AND, IntermediateRepresentation::i32, ops_dest_t, ops_r, ops_l);
                    my_function->insertStatement(tempVar0);
                    ops_dest = ops_dest_t;
                }
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest_t(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar0(IntermediateRepresentation::AND, IntermediateRepresentation::i32, ops_dest_t, ops_l, ops_r);
                my_function->insertStatement(tempVar0);
                ops_dest = ops_dest_t;
            }
            IntermediateRepresentation::Statement tempVar2(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_lab_z);
            my_function->insertStatement(tempVar2);

            IntermediateRepresentation::Statement tempVar3(IntermediateRepresentation::LABEL, IntermediateRepresentation::t_void, ops_lab_y);
            my_function->insertStatement(tempVar3);
            IntermediateRepresentation::IROperand ops_Imm0(IntermediateRepresentation::i32, 0);
            IntermediateRepresentation::Statement tempVar4(IntermediateRepresentation::MOV, IntermediateRepresentation::i32, ops_dest, ops_Imm0);
            my_function->insertStatement(tempVar4);
            my_function->insertStatement(tempVar2);

            IntermediateRepresentation::Statement tempVar5(IntermediateRepresentation::LABEL, IntermediateRepresentation::t_void, ops_lab_z);
            my_function->insertStatement(tempVar5);
            return ops_dest;
        }
    }
    else if(cond_operator == "OR_OP")
    {
        ops_l = pri_cond(a->left);

        if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal && ops_l.getValue())
        {
            IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, 1);
            return ops_dest;
        }
        else
        {
            std::string label_x = getNewLabel();
            IntermediateRepresentation::IROperand ops_lab_x(label_x);
            std::string label_y = getNewLabel();
            IntermediateRepresentation::IROperand ops_lab_y(label_y);
            std::string label_z = getNewLabel();
            IntermediateRepresentation::IROperand ops_lab_z(label_z);
            IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());

            IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_l, ops_lab_x, ops_lab_y);
            my_function->insertStatement(tempVar);

            IntermediateRepresentation::Statement tempVar3(IntermediateRepresentation::LABEL, IntermediateRepresentation::t_void, ops_lab_x);
            my_function->insertStatement(tempVar3);
            IntermediateRepresentation::IROperand ops_Imm1(IntermediateRepresentation::i32, 1);
            IntermediateRepresentation::Statement tempVar4(IntermediateRepresentation::MOV, IntermediateRepresentation::i32, ops_dest, ops_Imm1);
            my_function->insertStatement(tempVar4);
            IntermediateRepresentation::Statement tempVar2(IntermediateRepresentation::BR, IntermediateRepresentation::t_void, ops_lab_z);
            my_function->insertStatement(tempVar2);

            IntermediateRepresentation::Statement tempVar1(IntermediateRepresentation::LABEL, IntermediateRepresentation::t_void, ops_lab_y);
            my_function->insertStatement(tempVar1);
            ops_r = pri_cond(a->right);
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                if(ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    int value_l = ops_l.getValue();
                    int value_r = ops_r.getValue();
                    IntermediateRepresentation::IROperand ops_dest_t(IntermediateRepresentation::i32, value_l||value_r);
                    ops_dest = ops_dest_t;
                }
                else
                {
                    IntermediateRepresentation::Statement tempVar0(IntermediateRepresentation::OR, IntermediateRepresentation::i32, ops_dest, ops_r, ops_l);
                    my_function->insertStatement(tempVar0);
                }
            }
            else
            {
                IntermediateRepresentation::Statement tempVar0(IntermediateRepresentation::OR, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar0);
            }
            my_function->insertStatement(tempVar2);

            IntermediateRepresentation::Statement tempVar5(IntermediateRepresentation::LABEL, IntermediateRepresentation::t_void, ops_lab_z);
            my_function->insertStatement(tempVar5);
            return ops_dest;
        }
    }
    else
    {
        if(a->left)
        {
            ops_l = pri_cond(a->left);
        }
        else
        {
            IntermediateRepresentation::IROperand ops_Imm0(IntermediateRepresentation::i32, 0);
            ops_l = ops_Imm0;
        }
        if(a->right)
        {
            ops_r = pri_cond(a->right);
        }
        else
        {
            IntermediateRepresentation::IROperand ops_Imm0(IntermediateRepresentation::i32, 0);
            ops_r = ops_Imm0;
        }

        if(cond_operator == "LT")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                if(ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    int value_l = ops_l.getValue();
                    int value_r = ops_r.getValue();
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l<value_r);
                    return ops_dest;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CMP_SGT, IntermediateRepresentation::i32, ops_dest, ops_r, ops_l);
                    my_function->insertStatement(tempVar);
                    return ops_dest;
                }
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CMP_SLT, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(cond_operator == "GT")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                if(ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    int value_l = ops_l.getValue();
                    int value_r = ops_r.getValue();
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l>value_r);
                    return ops_dest;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CMP_SLT, IntermediateRepresentation::i32, ops_dest, ops_r, ops_l);
                    my_function->insertStatement(tempVar);
                    return ops_dest;
                }
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CMP_SGT, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(cond_operator == "LE_OP")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                if(ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    int value_l = ops_l.getValue();
                    int value_r = ops_r.getValue();
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l<=value_r);
                    return ops_dest;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CMP_SGE, IntermediateRepresentation::i32, ops_dest, ops_r, ops_l);
                    my_function->insertStatement(tempVar);
                    return ops_dest;
                }
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CMP_SLE, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(cond_operator == "GE_OP")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                if(ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    int value_l = ops_l.getValue();
                    int value_r = ops_r.getValue();
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l>=value_r);
                    return ops_dest;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CMP_SLE, IntermediateRepresentation::i32, ops_dest, ops_r, ops_l);
                    my_function->insertStatement(tempVar);
                    return ops_dest;
                }
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CMP_SGE, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(cond_operator == "EQ_OP")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                if(ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    int value_l = ops_l.getValue();
                    int value_r = ops_r.getValue();
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l==value_r);
                    return ops_dest;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CMP_EQ, IntermediateRepresentation::i32, ops_dest, ops_r, ops_l);
                    my_function->insertStatement(tempVar);
                    return ops_dest;
                }
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CMP_EQ, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(cond_operator == "NE_OP")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                if(ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    int value_l = ops_l.getValue();
                    int value_r = ops_r.getValue();
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l!=value_r);
                    return ops_dest;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CMP_NE, IntermediateRepresentation::i32, ops_dest, ops_r, ops_l);
                    my_function->insertStatement(tempVar);
                    return ops_dest;
                }
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CMP_NE, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(cond_operator == "NOT")
        {
            if(ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                int value_r = ops_r.getValue();
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, !value_r);
                return ops_dest;
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::NOT, IntermediateRepresentation::i32, ops_dest, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(cond_operator == "ADD")
        {
            if(ops_l.getValue() == 0 && ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                return ops_r;
            }
            else if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                if(ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    int value_l = ops_l.getValue();
                    int value_r = ops_r.getValue();
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l+value_r);
                    return ops_dest;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::ADD, IntermediateRepresentation::i32, ops_dest, ops_r, ops_l);
                    my_function->insertStatement(tempVar);
                    return ops_dest;
                }
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::ADD, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(cond_operator == "SUB")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal && ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                int value_l = ops_l.getValue();
                int value_r = ops_r.getValue();
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l-value_r);
                return ops_dest;
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::SUB, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(cond_operator == "MUL")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                if(ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    int value_l = ops_l.getValue();
                    int value_r = ops_r.getValue();
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l*value_r);
                    return ops_dest;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MUL, IntermediateRepresentation::i32, ops_dest, ops_r, ops_l);
                    my_function->insertStatement(tempVar);
                    return ops_dest;
                }
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MUL, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(cond_operator == "DIV")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal && ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                int value_l = ops_l.getValue();
                int value_r = ops_r.getValue();
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l/value_r);
                return ops_dest;
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::DIV, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(cond_operator == "MOD")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal && ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                int value_l = ops_l.getValue();
                int value_r = ops_r.getValue();
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l%value_r);
                return ops_dest;
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MOD, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else
        {
            std::cout << "error at pri_cond :" << cond_operator << std::endl;
            IntermediateRepresentation::IROperand ops_0(IntermediateRepresentation::i32, 0);
            return ops_0;
        }
    }
}

void pri_no_return_func(AST* a)
{
    std::string name = a->left->content;
    AST* temp = a->right;
    IntermediateRepresentation::IROperand my_ops0;
    //std::cout<< my_ops0.getIrDataType()<<"; "<< my_ops0.getIrOpType()<<std::endl;
    IntermediateRepresentation::IROperand my_ops1(name);
    if(temp)
    {
        std::vector<IntermediateRepresentation::IROperand> my_ops;
        my_ops.push_back(my_ops0);
        my_ops.push_back(my_ops1);
        //IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CALL, IntermediateRepresentation::t_void, my_ops0, my_ops1);
        std::stack<AST*> param_list_stack;
        while(temp)
        {
            param_list_stack.push(temp);
            temp = temp->left;
        }
        int index = 0;
        int i = symTab->findFunc(name);
        std::vector<int> value;
        //It's terrible
        if(i>=0)
        {
            value = symTab->getFunc(i).value;
        }
        else
        {
            value.push_back(1);
        }
        //terrible end

        while(!param_list_stack.empty())
        {
            temp = param_list_stack.top()->right;
            param_list_stack.pop();
            // var, array or exp
            if(temp->name == "IDENTIFIER")
            {
                std::string param_name = temp->content;
                if(value[index]==1)
                {
                    index++;
                    param_name = symTab->find_name(temp->content, symbalTableMember::INT);
                    IntermediateRepresentation::IROperand my_ops_more(IntermediateRepresentation::i32, param_name);
                    my_ops.push_back(my_ops_more);
                }
                else if(value[index]==0)
                {
                    index++;
                    param_name = symTab->find_name(temp->content, symbalTableMember::ARRAY);
                    IntermediateRepresentation::IROperand my_ops_more(IntermediateRepresentation::i32, param_name, true);
                    my_ops.push_back(my_ops_more);
                }
                else
                {
                    std::cout << "error at trans_block func_postfix_expression no type" << std::endl;
                    exit(-1);
                }
            }
            else
            {
                if(value[index]==1)
                {
                    index++;
                    IntermediateRepresentation::IROperand my_ops_more = pri_exp(temp);
                    my_ops.push_back(my_ops_more);
                }
                else if(value[index]==0)
                {
                    index++;
                    std::stack<AST*> arr_list;
                    while(temp)
                    {
                        arr_list.push(temp);
                        temp = temp->left;
                    }
                    std::string name = arr_list.top()->content;
                    arr_list.pop();
                    symbalTableMember tempTabVar = symTab->find(name, symbalTableMember::ARRAY);
                    std::string s_name = symTab->find_name(name, symbalTableMember::ARRAY);
                    int indexNum = tempTabVar.arrayIndex[0];
                    int num = 1;
                    IntermediateRepresentation::IROperand ops_temp0(IntermediateRepresentation::i32, s_name, true);
                    IntermediateRepresentation::IROperand ops_temp1(IntermediateRepresentation::i32, getNewNameLocalVar(), true);
                    //int off = 0;
                    IntermediateRepresentation::IROperand ops_off_temp(IntermediateRepresentation::i32, getNewNameLocalVar());
                    IntermediateRepresentation::IROperand ops_j;
                    while(!arr_list.empty())
                    {
                        temp = arr_list.top()->right;
                        arr_list.pop();
                        num++;
                        if(temp->name == "CONSTANT")
                        {
                            IntermediateRepresentation::IROperand ops_num(IntermediateRepresentation::i32, temp->value);
                            ops_j = ops_num;
                        }
                        else
                        {
                            ops_j = pri_exp(temp);
                        }

                        int size=1;
                        for(int i = num; i <= indexNum; i++)
                        {
                            size *= tempTabVar.arrayIndex[i];
                        }

                        if(ops_j.getIrOpType() == IntermediateRepresentation::ImmVal)
                        {
                            int num = ops_j.getValue();
                            ops_j.setValue(num*size);
                        }
                        else
                        {
                            IntermediateRepresentation::IROperand ops_temp(IntermediateRepresentation::i32, getNewNameLocalVar());
                            IntermediateRepresentation::IROperand ops_size(IntermediateRepresentation::i32, size);
                            IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MUL, IntermediateRepresentation::i32, ops_temp, ops_j, ops_size);
                            my_function->insertStatement(tempVar);
                            ops_j = ops_temp;
                        }

                        if(tempTabVar.arrayIndex[0] == 2)
                        {
                            ops_off_temp = ops_j;
                        }
                        else if(num == 2)
                        {
                            IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MOV, IntermediateRepresentation::i32, ops_off_temp, ops_j);
                            my_function->insertStatement(tempVar);
                        }
                        else
                        {
                            IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::ADD, IntermediateRepresentation::i32, ops_off_temp, ops_off_temp, ops_j);
                            my_function->insertStatement(tempVar);
                        }
                    }
                    IntermediateRepresentation::IROperand ops_off;
                    if(ops_off_temp.getIrOpType() == IntermediateRepresentation::ImmVal)
                    {
                        IntermediateRepresentation::IROperand ops_temp(IntermediateRepresentation::i32, ops_off_temp.getValue()*4);
                        ops_off = ops_temp;
                    }
                    else
                    {
                        IntermediateRepresentation::IROperand ops_off1(IntermediateRepresentation::i32, getNewNameLocalVar());
                        ops_off = ops_off1;
                        IntermediateRepresentation::IROperand ops_Imm4(IntermediateRepresentation::i32, 4);
                        IntermediateRepresentation::Statement tempVar1(IntermediateRepresentation::MUL, IntermediateRepresentation::i32, ops_off, ops_off_temp, ops_Imm4);
                        my_function->insertStatement(tempVar1);
                    }
                    
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::ADD, IntermediateRepresentation::i32, ops_temp1, ops_temp0, ops_off);
                    my_function->insertStatement(tempVar);

                    my_ops.push_back(ops_temp1);
                }
            }
            
        }
        IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CALL, IntermediateRepresentation::t_void, my_ops);
        my_function->insertStatement(tempVar);
    }
    else //no param
    {
        IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CALL, IntermediateRepresentation::t_void, my_ops0, my_ops1);
        my_function->insertStatement(tempVar);
    }
}

IntermediateRepresentation::IROperand pri_return_func(AST* a)
{
    std::string name = a->left->content;
    AST* temp = a->right;
    IntermediateRepresentation::IROperand my_ops0(IntermediateRepresentation::i32, getNewNameLocalVar());
    //std::cout<< my_ops0.getIrDataType()<<"; "<< my_ops0.getIrOpType()<<std::endl;
    IntermediateRepresentation::IROperand my_ops1(name);
    if(temp)
    {
        std::vector<IntermediateRepresentation::IROperand> my_ops;
        my_ops.push_back(my_ops0);
        my_ops.push_back(my_ops1);
        
        std::stack<AST*> param_list_stack;
        while(temp)
        {
            param_list_stack.push(temp);
            temp = temp->left;
        }
        int index = 0;
        int i = symTab->findFunc(name);
        std::vector<int> value;
        //It's terrible
        if(i>=0)
        {
            value = symTab->getFunc(i).value;
        }
        else
        {
            value.push_back(1);
        }
        //

        while(!param_list_stack.empty())
        {
            temp = param_list_stack.top()->right;
            param_list_stack.pop();

            // var, array or exp
            if(temp->name == "IDENTIFIER")
            {
                std::string param_name = temp->content;
                if(value[index]==1)
                {
                    index++;
                    param_name = symTab->find_name(temp->content, symbalTableMember::INT);
                    IntermediateRepresentation::IROperand my_ops_more(IntermediateRepresentation::i32, param_name);
                    my_ops.push_back(my_ops_more);
                }
                else if(value[index]==0)
                {
                    index++;
                    param_name = symTab->find_name(temp->content, symbalTableMember::ARRAY);
                    IntermediateRepresentation::IROperand my_ops_more(IntermediateRepresentation::i32, param_name, true);
                    my_ops.push_back(my_ops_more);
                }
                else
                {
                    std::cout << "error at trans_block func_postfix_expression no type" << std::endl;
                    exit(-1);
                }
            }
            else
            {
                if(value[index]==1) //int
                {
                    index++;
                    IntermediateRepresentation::IROperand my_ops_more = pri_exp(temp);
                    my_ops.push_back(my_ops_more);
                }
                else if(value[index]==0)
                {
                    index++;
                    std::stack<AST*> arr_list;
                    while(temp)
                    {
                        arr_list.push(temp);
                        temp = temp->left;
                    }
                    std::string name = arr_list.top()->content;
                    arr_list.pop();
                    symbalTableMember tempTabVar = symTab->find(name, symbalTableMember::ARRAY);
                    std::string s_name = symTab->find_name(name, symbalTableMember::ARRAY);
                    int indexNum = tempTabVar.arrayIndex[0];
                    int num = 1;
                    IntermediateRepresentation::IROperand ops_temp0(IntermediateRepresentation::i32, s_name, true);
                    IntermediateRepresentation::IROperand ops_temp1(IntermediateRepresentation::i32, getNewNameLocalVar(), true);
                    //int off = 0;
                    IntermediateRepresentation::IROperand ops_off_temp(IntermediateRepresentation::i32, getNewNameLocalVar());
                    IntermediateRepresentation::IROperand ops_j;
                    while(!arr_list.empty())
                    {
                        temp = arr_list.top()->right;
                        arr_list.pop();
                        num++;
                        if(temp->name == "CONSTANT")
                        {
                            IntermediateRepresentation::IROperand ops_num(IntermediateRepresentation::i32, temp->value);
                            ops_j = ops_num;
                        }
                        else
                        {
                            ops_j = pri_exp(temp);
                        }

                        int size=1;
                        for(int i = num; i <= indexNum; i++)
                        {
                            size *= tempTabVar.arrayIndex[i];
                        }

                        if(ops_j.getIrOpType() == IntermediateRepresentation::ImmVal)
                        {
                            int num = ops_j.getValue();
                            ops_j.setValue(num*size);
                        }
                        else
                        {
                            IntermediateRepresentation::IROperand ops_temp(IntermediateRepresentation::i32, getNewNameLocalVar());
                            IntermediateRepresentation::IROperand ops_size(IntermediateRepresentation::i32, size);
                            IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MUL, IntermediateRepresentation::i32, ops_temp, ops_j, ops_size);
                            my_function->insertStatement(tempVar);
                            ops_j = ops_temp;
                        }

                        if(tempTabVar.arrayIndex[0] == 2)
                        {
                            ops_off_temp = ops_j;
                        }
                        else if(num == 2)
                        {
                            IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MOV, IntermediateRepresentation::i32, ops_off_temp, ops_j);
                            my_function->insertStatement(tempVar);
                        }
                        else
                        {
                            IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::ADD, IntermediateRepresentation::i32, ops_off_temp, ops_off_temp, ops_j);
                            my_function->insertStatement(tempVar);
                        }
                    }
                    IntermediateRepresentation::IROperand ops_off;
                    if(ops_off_temp.getIrOpType() == IntermediateRepresentation::ImmVal)
                    {
                        IntermediateRepresentation::IROperand ops_temp(IntermediateRepresentation::i32, ops_off_temp.getValue()*4);
                        ops_off = ops_temp;
                    }
                    else
                    {
                        IntermediateRepresentation::IROperand ops_off1(IntermediateRepresentation::i32, getNewNameLocalVar());
                        ops_off = ops_off1;
                        IntermediateRepresentation::IROperand ops_Imm4(IntermediateRepresentation::i32, 4);
                        IntermediateRepresentation::Statement tempVar1(IntermediateRepresentation::MUL, IntermediateRepresentation::i32, ops_off, ops_off_temp, ops_Imm4);
                        my_function->insertStatement(tempVar1);
                    }
                    
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::ADD, IntermediateRepresentation::i32, ops_temp1, ops_temp0, ops_off);
                    my_function->insertStatement(tempVar);

                    my_ops.push_back(ops_temp1);
                }
                else
                {
                    std::cout << "error at trans_block func_postfix_expression no type" << std::endl;
                    exit(-1);
                }
            }

        }
        IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CALL, IntermediateRepresentation::i32, my_ops);
        my_function->insertStatement(tempVar);
    }
    else //no param
    {
        IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::CALL, IntermediateRepresentation::t_void, my_ops0, my_ops1);
        my_function->insertStatement(tempVar);
    }

    return my_ops0;

}

IntermediateRepresentation::IROperand pri_const_var_exp(AST* a, bool isglobal)
{
    //return Imm
    std::string exp_operator = a->name;
    IntermediateRepresentation::IROperand ops_l;
    IntermediateRepresentation::IROperand ops_r;
    
    if(a == NULL)
    {
        IntermediateRepresentation::IROperand ops_Imm0(IntermediateRepresentation::i32, 0);
        return ops_Imm0;
    }

    if(exp_operator == "IDENTIFIER")
    {
        int value;
        if(isglobal)
        {
            int i = symTab->findInGlobal(a->content, symbalTableMember::INT);
            symbalTableMember symTabTemp = symTab->getGlobalVar(i);
            value = symTabTemp.value.back();
        }
        else
        {
            symbalTableMember symTabTemp = symTab->find(a->content, symbalTableMember::INT);
            value = symTabTemp.value.back();
        }
        IntermediateRepresentation::IROperand ops_Imm(IntermediateRepresentation::i32, value);
        return ops_Imm;

    }
    else if(exp_operator == "CONSTANT")
    {
        IntermediateRepresentation::IROperand ops_Imm(IntermediateRepresentation::i32, a->value);
        return ops_Imm;
    }
    else if(exp_operator == "arr_postfix_expression")
    {
        std::cout<< "error at pri_const_var_exp" << std::endl; 
        return pri_arr_postfix_expression(a);
    }
    else if(exp_operator == "func_postfix_expression")
    {
        std::cout<< "error at pri_const_var_exp" << std::endl; 
        return pri_return_func(a);
    }
    else {
        
        if(a->left)
        {
            ops_l = pri_const_var_exp(a->left, isglobal);
        }
        else
        {
            IntermediateRepresentation::IROperand ops_Imm0(IntermediateRepresentation::i32, 0);
            ops_l = ops_Imm0;
        }
        if(a->right)
        {
            ops_r = pri_const_var_exp(a->right, isglobal);
        }
        else
        {
            IntermediateRepresentation::IROperand ops_Imm0(IntermediateRepresentation::i32, 0);
            ops_r = ops_Imm0;
        }

        if(exp_operator == "ADD")
        {
            if(ops_l.getValue() == 0 && ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                return ops_r;
            }
            else if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                if(ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    int value_l = ops_l.getValue();
                    int value_r = ops_r.getValue();
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l+value_r);
                    return ops_dest;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::ADD, IntermediateRepresentation::i32, ops_dest, ops_r, ops_l);
                    my_function->insertStatement(tempVar);
                    return ops_dest;
                }
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::ADD, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(exp_operator == "SUB")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal && ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                int value_l = ops_l.getValue();
                int value_r = ops_r.getValue();
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l-value_r);
                return ops_dest;
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::SUB, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(exp_operator == "MUL")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                if(ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    int value_l = ops_l.getValue();
                    int value_r = ops_r.getValue();
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l*value_r);
                    return ops_dest;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MUL, IntermediateRepresentation::i32, ops_dest, ops_r, ops_l);
                    my_function->insertStatement(tempVar);
                    return ops_dest;
                }
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MUL, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(exp_operator == "DIV")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal && ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                int value_l = ops_l.getValue();
                int value_r = ops_r.getValue();
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l/value_r);
                return ops_dest;
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::DIV, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(exp_operator == "MOD")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal && ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                int value_l = ops_l.getValue();
                int value_r = ops_r.getValue();
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l%value_r);
                return ops_dest;
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MOD, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else
        {
            std::cout<< "error at pri_const_var_exp" << std::endl; 
            IntermediateRepresentation::IROperand ops_0(IntermediateRepresentation::i32, 0);
            return ops_0;
        }
    }    
}

IntermediateRepresentation::IROperand pri_exp(AST* a)
{
    //ADD,SUB,MUL,DIV,MOD
    std::string exp_operator = a->name;
    IntermediateRepresentation::IROperand ops_l;
    IntermediateRepresentation::IROperand ops_r;
    //IDENTIFIER
    //CONSTANT
    //arr_postfix_expression
    //func_postfix_expression
    if(a == NULL)
    {
        IntermediateRepresentation::IROperand ops_Imm0(IntermediateRepresentation::i32, 0);
        return ops_Imm0;
    }

    if(exp_operator == "IDENTIFIER")
    {
        std::string s_name = symTab->find_name(a->content, symbalTableMember::INT);
        IntermediateRepresentation::IROperand ops_var(IntermediateRepresentation::i32, s_name);
        return ops_var;

    }
    else if(exp_operator == "CONSTANT")
    {
        IntermediateRepresentation::IROperand ops_Imm(IntermediateRepresentation::i32, a->value);
        return ops_Imm;
    }
    else if(exp_operator == "arr_postfix_expression")
    {
        return pri_arr_postfix_expression(a);
    }
    else if(exp_operator == "func_postfix_expression")
    {
        return pri_return_func(a);
    }
    else {
        
        if(a->left)
        {
            ops_l = pri_exp(a->left);
        }
        else
        {
            IntermediateRepresentation::IROperand ops_Imm0(IntermediateRepresentation::i32, 0);
            ops_l = ops_Imm0;
        }
        if(a->right)
        {
            ops_r = pri_exp(a->right);
        }
        else
        {
            IntermediateRepresentation::IROperand ops_Imm0(IntermediateRepresentation::i32, 0);
            ops_r = ops_Imm0;
        }

        if(exp_operator == "ADD")
        {
            if(ops_l.getValue() == 0 && ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                return ops_r;
            }
            else if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                if(ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    int value_l = ops_l.getValue();
                    int value_r = ops_r.getValue();
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l+value_r);
                    return ops_dest;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::ADD, IntermediateRepresentation::i32, ops_dest, ops_r, ops_l);
                    my_function->insertStatement(tempVar);
                    return ops_dest;
                }
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::ADD, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(exp_operator == "SUB")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal && ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                int value_l = ops_l.getValue();
                int value_r = ops_r.getValue();
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l-value_r);
                return ops_dest;
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::SUB, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(exp_operator == "MUL")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                if(ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    int value_l = ops_l.getValue();
                    int value_r = ops_r.getValue();
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l*value_r);
                    return ops_dest;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MUL, IntermediateRepresentation::i32, ops_dest, ops_r, ops_l);
                    my_function->insertStatement(tempVar);
                    return ops_dest;
                }
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MUL, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(exp_operator == "DIV")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal && ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                int value_l = ops_l.getValue();
                int value_r = ops_r.getValue();
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l/value_r);
                return ops_dest;
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::DIV, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else if(exp_operator == "MOD")
        {
            if(ops_l.getIrOpType() == IntermediateRepresentation::ImmVal && ops_r.getIrOpType() == IntermediateRepresentation::ImmVal)
            {
                int value_l = ops_l.getValue();
                int value_r = ops_r.getValue();
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, value_l%value_r);
                return ops_dest;
            }
            else
            {
                IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MOD, IntermediateRepresentation::i32, ops_dest, ops_l, ops_r);
                my_function->insertStatement(tempVar);
                return ops_dest;
            }
        }
        else
        {
            std::cout << "error at pri_exp" << std::endl;
            IntermediateRepresentation::IROperand ops_0(IntermediateRepresentation::i32, 0);
            return ops_0;
        }
    }    
}

IntermediateRepresentation::IROperand pri_arr_postfix_expression(AST* a)
{
    AST* temp = a;
    std::stack<AST*> exp_stack;
    while(temp)
    {
        exp_stack.push(temp);
        temp = temp->left;
    }
    std::string arrName = exp_stack.top()->content;
    exp_stack.pop();
    symbalTableMember tempTabVar = symTab->find(arrName, symbalTableMember::ARRAY);
    arrName = symTab->find_name(arrName, symbalTableMember::ARRAY);
    int indexNum = tempTabVar.arrayIndex[0];
    int num = 1;
    IntermediateRepresentation::IROperand ops_temp0(IntermediateRepresentation::i32, getNewNameLocalVar());
    while(!exp_stack.empty())
    {
        temp = exp_stack.top()->right;
        exp_stack.pop();
        //do something
        IntermediateRepresentation::IROperand ops_temp1 = pri_exp(temp);
        if(indexNum == 1)
        {
            ops_temp0 = ops_temp1;
        }
        else
        {
            //std::cout << indexNum << std::endl;
            IntermediateRepresentation::IROperand ops_temp2(IntermediateRepresentation::i32, getNewNameLocalVar());
            num++;
            int j = 1;
            for(int i = num; i <= indexNum; i++)
            {
                j *= tempTabVar.arrayIndex[i];
            }
            if(j == 1)
            {
                ops_temp2 = ops_temp1;
            }
            else
            {
                if(ops_temp1.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    int value = ops_temp1.getValue();
                    IntermediateRepresentation::IROperand ops_num(IntermediateRepresentation::i32, value*j);
                    ops_temp2 = ops_num;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_j(IntermediateRepresentation::i32, j);
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MUL, IntermediateRepresentation::i32, ops_temp2, ops_temp1, ops_j);
                    my_function->insertStatement(tempVar);
                }
            }
            
            if(num == 2)
            {
                //ops_temp0 = ops_temp2;
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MOV, IntermediateRepresentation::i32, ops_temp0, ops_temp2);
                my_function->insertStatement(tempVar);
            }
            else
            {
                if(ops_temp0.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    if(ops_temp2.getIrOpType() == IntermediateRepresentation::ImmVal)
                    {
                        int value1 = ops_temp0.getValue();
                        int value2 = ops_temp2.getValue();
                        IntermediateRepresentation::IROperand ops_num(IntermediateRepresentation::i32, value1+value2);
                        ops_temp0 = ops_num;
                    }
                    else
                    {
                        IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::ADD, IntermediateRepresentation::i32, ops_temp0, ops_temp2, ops_temp0);
                        my_function->insertStatement(tempVar);
                    }
                }
                else
                {
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::ADD, IntermediateRepresentation::i32, ops_temp0, ops_temp0, ops_temp2);
                    my_function->insertStatement(tempVar);
                }
                
            }
        }
    }
    
    IntermediateRepresentation::IROperand ops_off(IntermediateRepresentation::i32, getNewNameLocalVar());
    if(ops_temp0.getIrOpType() == IntermediateRepresentation::ImmVal)
    {
        int value = ops_temp0.getValue();
        IntermediateRepresentation::IROperand ops_num(IntermediateRepresentation::i32, value*4);
        /*
        IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MOV, IntermediateRepresentation::i32, ops_off, ops_num);
        my_function->insertStatement(tempVar);
        */
       ops_off = ops_num;
    }
    else
    {
        IntermediateRepresentation::IROperand ops_num_4(IntermediateRepresentation::i32, 4);
        IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MUL, IntermediateRepresentation::i32, ops_off, ops_temp0, ops_num_4);
        my_function->insertStatement(tempVar);
    }
    
    IntermediateRepresentation::IROperand ops_dest(IntermediateRepresentation::i32, getNewNameLocalVar());
    IntermediateRepresentation::IROperand ops_base(IntermediateRepresentation::i32, arrName, true);
    IntermediateRepresentation::Statement tempVar1(IntermediateRepresentation::LOAD, IntermediateRepresentation::i32, ops_dest, ops_base, ops_off);
    my_function->insertStatement(tempVar1);

    return ops_dest;
}

void pri_LVal_arr_postfix_expression(AST* LVal, AST* exp)
{
    AST* temp = LVal;
    std::stack<AST*> exp_stack;
    while(temp)
    {
        exp_stack.push(temp);
        temp = temp->left;
    }
    std::string arrName = exp_stack.top()->content;
    exp_stack.pop();
    symbalTableMember tempTabVar = symTab->find(arrName, symbalTableMember::ARRAY);
    arrName = symTab->find_name(arrName, symbalTableMember::ARRAY);
    int indexNum = tempTabVar.arrayIndex[0];
    int num = 1;
    IntermediateRepresentation::IROperand ops_temp0(IntermediateRepresentation::i32, getNewNameLocalVar());
    while(!exp_stack.empty())
    {
        temp = exp_stack.top()->right;
        exp_stack.pop();
        //do something
        IntermediateRepresentation::IROperand ops_temp1 = pri_exp(temp);
        if(indexNum == 1)
        {
            ops_temp0 = ops_temp1;
        }
        else
        {
            //std::cout << indexNum << std::endl;
            IntermediateRepresentation::IROperand ops_temp2(IntermediateRepresentation::i32, getNewNameLocalVar());
            num++;
            int j = 1;
            for(int i = num; i <= indexNum; i++)
            {
                j *= tempTabVar.arrayIndex[i];
            }
            if(j == 1)
            {
                ops_temp2 = ops_temp1;
            }
            else
            {
                if(ops_temp1.getIrOpType() == IntermediateRepresentation::ImmVal)
                {
                    int value = ops_temp1.getValue();
                    IntermediateRepresentation::IROperand ops_num(IntermediateRepresentation::i32, value*j);
                    ops_temp2 = ops_num;
                }
                else
                {
                    IntermediateRepresentation::IROperand ops_j(IntermediateRepresentation::i32, j);
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MUL, IntermediateRepresentation::i32, ops_temp2, ops_temp1, ops_j);
                    my_function->insertStatement(tempVar);
                }
            }

            if(num == 2)
            {
                //ops_temp0 = ops_temp2;
                IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MOV, IntermediateRepresentation::i32, ops_temp0, ops_temp2);
                my_function->insertStatement(tempVar);
            }
            else
            {
                if(ops_temp0.getIrOpType() == IntermediateRepresentation::ImmVal) //error
                {
                   if(ops_temp2.getIrOpType() == IntermediateRepresentation::ImmVal)
                    {
                        int value1 = ops_temp0.getValue();
                        int value2 = ops_temp2.getValue();
                        IntermediateRepresentation::IROperand ops_num(IntermediateRepresentation::i32, value1+value2);
                        ops_temp0 = ops_num;
                    }
                    else
                    {
                        IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::ADD, IntermediateRepresentation::i32, ops_temp0, ops_temp2, ops_temp0);
                        my_function->insertStatement(tempVar);
                    }
                }
                else
                {
                    IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::ADD, IntermediateRepresentation::i32, ops_temp0, ops_temp0, ops_temp2);
                    my_function->insertStatement(tempVar);
                }
            }
        }
    }

    IntermediateRepresentation::IROperand ops_off(IntermediateRepresentation::i32, getNewNameLocalVar());
    if(ops_temp0.getIrOpType() == IntermediateRepresentation::ImmVal)
    {
        int value = ops_temp0.getValue();
        IntermediateRepresentation::IROperand ops_num(IntermediateRepresentation::i32, value*4);
        ops_off = ops_num;
    }
    else
    {
        IntermediateRepresentation::IROperand ops_num_4(IntermediateRepresentation::i32, 4);
        IntermediateRepresentation::Statement tempVar(IntermediateRepresentation::MUL, IntermediateRepresentation::i32, ops_off, ops_temp0, ops_num_4);
        my_function->insertStatement(tempVar);
    }
    
    IntermediateRepresentation::IROperand ops_src = pri_exp(exp);
    IntermediateRepresentation::IROperand ops_base(IntermediateRepresentation::i32, arrName, true);
    IntermediateRepresentation::Statement tempVar1(IntermediateRepresentation::STORE, IntermediateRepresentation::i32, ops_src, ops_base, ops_off);
    my_function->insertStatement(tempVar1);

}

std::string getNewNameLocalVar()
{
    std::string name = "var_"+std::to_string(localVarNum);
    localVarNum++;

    return name;
}

std::string getNewLabel()
{
    labelNum++;
    std::string label = "label_"+std::to_string(labelNum);

    return label;
}

void sysy_runingtime_func_init()
{
    symbalTableMember symTabM1;
    symTabM1.init("getint", "getint", symbalTableMember::FUNC, 0);
    symTab->addFunc(symTabM1);

    symbalTableMember symTabM2;
    symTabM2.init("getch", "getch", symbalTableMember::FUNC, 0);
    symTab->addFunc(symTabM2);

    symbalTableMember symTabM3;
    symTabM3.init("getarray", "getarray", symbalTableMember::FUNC, 0);
    std::vector<int> value3;
    value3.push_back(0); 
    symTabM3.value = value3;
    symTab->addFunc(symTabM3);

    symbalTableMember symTabM4;
    symTabM4.init("putint", "putint", symbalTableMember::FUNC, 0);
    std::vector<int> value4;
    value4.push_back(1);
    symTabM4.value = value4;
    symTab->addFunc(symTabM4);

    symbalTableMember symTabM5;
    symTabM5.init("putch", "putch", symbalTableMember::FUNC, 0);
    std::vector<int> value5;
    value5.push_back(1);
    symTabM5.value = value5;
    symTab->addFunc(symTabM5);

    symbalTableMember symTabM6;
    symTabM6.init("putarray", "putarray", symbalTableMember::FUNC, 0);
    std::vector<int> value6;
    value6.push_back(1);
    value6.push_back(0);
    symTabM6.value = value6;
    symTab->addFunc(symTabM6);

    symbalTableMember symTabM7;
    symTabM7.init("_sysy_starttime", "_sysy_starttime", symbalTableMember::FUNC, 0);
    std::vector<int> value7;
    value7.push_back(1);
    symTabM7.value = value7;
    symTab->addFunc(symTabM7);

    symbalTableMember symTabM8;
    symTabM8.init("_sysy_stoptime", "_sysy_stoptime", symbalTableMember::FUNC, 0);
    std::vector<int> value8;
    value8.push_back(1);
    symTabM8.value = value8;
    symTab->addFunc(symTabM8);
}