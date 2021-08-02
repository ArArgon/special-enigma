%option yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "lrparser.tab.hpp"

static FILE* yyparse_file_ptr;
%}

letter [a-zA-Z]
digit [0-9]
decimal (0|[1-9][0-9]*)
octal 0[0-7]+
hexadecimal 0[xX][a-fA-F0-9]+
CONSTANT ({decimal}|{hexadecimal}|{octal})
delim [ \t\r\f\v\n]+
ID ({letter}|_)({letter}|{digit}|_)*
COMMENT_1  \/\*([^*]|\*+[^/*])*\*+\/
COMMENT_2  \/\/[^\n]*


%%

"break" {yylval.node=new_node("BREAK",0,yylineno); return (BREAK);}
"const" {yylval.node=new_node("CONST",0,yylineno); return (CONST);}
"continue" {yylval.node=new_node("CONTINUE",0,yylineno); return (CONTINUE);}
"else" {yylval.node=new_node("ELSE",0,yylineno); return (ELSE);}
"if" {yylval.node=new_node("IF",0,yylineno); return (IF);}
"while" {yylval.node=new_node("WHILE",0,yylineno); return (WHILE);}
"return" {yylval.node=new_node("RETURN",0,yylineno); return (RETURN);}
"int" {yylval.node=new_node("INT",0,yylineno); return (INT);}
"void" {yylval.node=new_node("VOID",0,yylineno); return (VOID);}

"{" {yylval.node=new_node("LB",0,yylineno); return (LB);}
"[" {yylval.node=new_node("LA",0,yylineno); return (LA);}
"(" {yylval.node=new_node("LP",0,yylineno); return (LP);}
"}" {yylval.node=new_node("RB",0,yylineno); return (RB);}
"]" {yylval.node=new_node("RA",0,yylineno); return (RA);}
")" {yylval.node=new_node("RP",0,yylineno); return (RP);}
"/" {yylval.node=new_node("DIV",0,yylineno); return (DIV);}
"%" {yylval.node=new_node("MOD",0,yylineno); return (MOD);}
"!" {yylval.node=new_node("NOT",0,yylineno); return (NOT);}
"&&" {yylval.node=new_node("AND_OP",0,yylineno); return (AND_OP);}
"||" {yylval.node=new_node("OR_OP",0,yylineno); return (OR_OP);}
"<" {yylval.node=new_node("LT",0,yylineno); return (LT);}
"<=" {yylval.node=new_node("LE_OP",0,yylineno); return (LE_OP);}
">" {yylval.node=new_node("GT",0,yylineno); return (GT);}
">=" {yylval.node=new_node("GE_OP",0,yylineno); return (GE_OP);}
"==" {yylval.node=new_node("EQ_OP",0,yylineno); return (EQ_OP);}
"!=" {yylval.node=new_node("NE_OP",0,yylineno); return (NE_OP);}
"=" {yylval.node=new_node("ASSIGN",0,yylineno); return (ASSIGN);}
"-" {yylval.node=new_node("SUB",0,yylineno); return (SUB);}
"+" {yylval.node=new_node("ADD",0,yylineno); return (ADD);}
"*" {yylval.node=new_node("MUL",0,yylineno); return (MUL);}
"," {yylval.node=new_node("COMMA",0,yylineno); return (COMMA);}
";" {yylval.node=new_node("SC",0,yylineno); return (SC);}

{COMMENT_1} {}
{COMMENT_2} {}
{delim} {}
{ID} {yylval.node=new_node("IDENTIFIER",0,yylineno); return (IDENTIFIER);}
{CONSTANT} {yylval.node=new_node("CONSTANT",0,yylineno); return (CONSTANT);}

%%

int yywrap() 
{
	return 1;
}

void yyparse_init(const char* filename)
{
	yyparse_file_ptr = fopen(filename, "r");
	yyin = yyparse_file_ptr;
}

void yyparse_cleanup()
{
	fclose(yyparse_file_ptr);
}