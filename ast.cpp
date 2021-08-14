#include "ast.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <stdarg.h>

struct AST *new_node(std::string name,int num,...)
{
    va_list valist;//定义变长参数列表
    struct AST *newnode = new AST;
    struct AST *temp = new AST;
    
    if(!newnode)
    {
        yyerror("out of space");
        exit(0);
    }

    newnode->name = name;

    va_start(valist,num);//初始化变长参数为num后的参数
    if(num > 0)
    {
        temp = va_arg(valist, struct AST*);
        newnode->left = temp;
        newnode->right = NULL;
        newnode->line = temp->line;
        if(num == 1)
        {
            newnode->content = temp->content;
        }
        else
        {
            for(int i=0; i<num-1; i++)
            {
                temp->right = va_arg(valist, struct AST*);
                temp = temp->right;
            }
        }
    }
    else //num==0为终结符(或产生空的语法单元)
    {
        int t = va_arg(valist, int);
        newnode->line = t; //第1个变长参数表示行号(,产生空的语法单元行号为-1)

        newnode->left = NULL;
        newnode->right = NULL;

        if(newnode->name=="CONSTANT")
        {
            newnode->type = "int";
            newnode->value = strtol(yytext, NULL, 0);
            //std::cout <<yytext<< std::endl;
            //std::cout <<newnode->value<< std::endl;

        }
        else
        {
            std::string yycontent = yytext;
            if(yycontent == "starttime")
                newnode->content = "_sysy_starttime";
            else if(yycontent == "stoptime")
                newnode->content = "_sysy_stoptime";
            else
                newnode->content = yycontent;
        }
    }

    return newnode;
}

struct AST *new_node_r(std::string name, struct AST* temp)
{
    struct AST *newnode = new AST;
    
    if(!newnode)
    {
        yyerror("out of space");
        exit(0);
    }

    newnode->name = name;

    newnode->left = NULL;
    newnode->right = temp;
    if(temp)
    {
        newnode->line = temp->line;
        newnode->content = temp->content;
    }

    return newnode;
}

void showast(struct AST *node,int level)//先序遍历抽象语法树
{
    if(node!=NULL)
    {
        for(int i=0; i<level; i++)//孩子结点相对父节点缩进2个空格
            std::cout << "  " ;
        if(node->line!=-1)  //空的语法单元不打印
        {
            std::cout << node->name;//打印语法单元名字，ID/CONSTANT要打印yytext的值
            if(node->name=="IDENTIFIER")
                std::cout << ": " << node->content;
            else if(node->name=="CONSTANT")
                std::cout << ": " << node->value;
            else
                std::cout << " (" <<  node->line << ")";
        }
        std::cout << std::endl;
        showast(node->left, level+1);
        showast(node->right, level);
    }
    return;
}

void deleteAST(AST* a)
{
    if(!a)
    {
        deleteAST(a->left);
        deleteAST(a->right);

        delete a;

    }
    return;
}