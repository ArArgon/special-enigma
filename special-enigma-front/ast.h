#ifndef _AST_H_

#define _AST_H_

#include <stdio.h>
#include <string>

extern char *yytext; 
extern int yylineno; //行号
extern int yylex();
extern int yyparse();
void yyerror(const char*s);

struct AST {
    int line; //行号
    std::string name; //结点的内容，一般为空，除非为叶节点
    struct AST *left; //左孩子
    struct AST *right; //右兄弟
    std::string content;//语法单元语义值(例如int i;ID的content是‘i’)
    std::string type;//语法单元数据类型:主要用于等号和操作符左右类型匹配判断
    int value;//常数值
};

AST *parseAST(const char* filename);

/**************抽象语法树**************/
struct AST *new_node(std::string name,int num,...); //num:变长参数中语法结点个数
struct AST *new_node_r(std::string name, struct AST* temp); //添加temp为右孩子
void showast(struct AST *a,int level);//遍历抽象语法树，level为树的层数
void deleteAST(AST*);
#endif
