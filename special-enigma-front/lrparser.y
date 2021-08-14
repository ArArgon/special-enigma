%code requires{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

extern AST *astRoot;
void yyparse_init(const char* filename);
void yyparse_cleanup();
}

%union{
struct AST* node;
int d;
}

%token <node> VOID CONST INT RETURN WHILE BREAK IF ELSE CONTINUE
%token <node> IDENTIFIER CONSTANT
%token <node> LB "{"
%token <node> LA "["
%token <node> LP "("
%token <node> RB "}"
%token <node> RA "]"
%token <node> RP ")"
%token <node> COMMA ","
%token <node> SC ";"
%token <node> ADD "+"
%token <node> SUB "-"
%token <node> MUL "*"
%token <node> DIV "/"
%token <node> MOD "%"
%token <node> NOT "!"
%token <node> ASSIGN "="
%token <node> LT "<"
%token <node> GT ">"
%token <node> LE_OP "<="
%token <node> GE_OP ">="
%token <node> EQ_OP "=="
%token <node> NE_OP "!="
%token <node> AND_OP "&&" 
%token <node> OR_OP "||"
%type <node> compile_unit external_declaration declaration declaration_specifiers type_qualifier init_declarator_list 
init_declarator direct_declarator initializer initializer_list constant_expression type_specifier function_definition 
parameter_list parameter_declaration block blockitem_list blockitem statement expression_statement expression
assignment_expression conditional_expression logical_or_expression logical_and_expression equality_expression 
relational_expression additive_expression multiplicative_expression unary_expression postfix_expression primary_expression 
argument_expression_list unary_operator assignment_operator selection_statement iteration_statement jump_statement

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%start compile_unit

%%
compile_unit:
    compile_unit external_declaration {$$=new_node("compile_unit",1,$1); $$->right=$2; astRoot=$$;}
    | external_declaration {$$=new_node_r("compile_unit",$1); astRoot=$$;}
    ;

external_declaration: 
    function_definition {$$=$1;}
    | declaration {$$=$1;}
    ;

declaration: 
	declaration_specifiers ";" {$$=new_node("declaration",1,$1);}
	| declaration_specifiers init_declarator_list ";" {$$=new_node("declaration",1,$1); $$->right=$2;}
	;

declaration_specifiers: 
	type_specifier {$$=$1;}
    | type_specifier declaration_specifiers {$$=$1; $$->right=$1;}
	| type_qualifier {$$=$1;}
    | type_qualifier declaration_specifiers {$$=$2; $$->left=$1;}
	;

type_qualifier: 
    CONST {$$=$1;}
    ;

init_declarator_list: 
	init_declarator {$$=new_node_r("init_declarator_list",$1);}
	| init_declarator_list "," init_declarator {$$=new_node("init_declarator_list",1,$1); $$->right=$3;}
	;

init_declarator: 
	direct_declarator {$$=$1;}
	| direct_declarator "=" initializer {$$=$2; $$->left=$1; $$->right=$3;}
	;

direct_declarator: 
	IDENTIFIER {$$=$1;}
	| direct_declarator "[" constant_expression "]" {$$=new_node("direct_declarator",1,$1); $$->right=$3;}
	| direct_declarator "[" "]" {$$=new_node("direct_declarator",1,$1); $$->right=NULL;}
	| direct_declarator "(" parameter_list ")" {$$=new_node("direct_declarator",1,$1); $$->right=$3;}
	| direct_declarator "(" ")" {$$=new_node("direct_declarator",1,$1); $$->right=NULL;}
	;

initializer: 
	assignment_expression {$$=$1;} 
    | "{" "}" {$$=NULL;}
	| "{" initializer_list "}" {$$=$2;}
	;

initializer_list: 
	initializer {$$=new_node_r("initializer_list",$1);}
	| initializer_list "," initializer {$$=new_node("initializer_list",1,$1); $$->right=$3;}
	;

constant_expression: 
    conditional_expression {$$=$1;}
    ;

type_specifier: 
    VOID {$$=$1;}
    | INT {$$=$1;}
    ;

function_definition: 
	declaration_specifiers direct_declarator block {$$=new_node("function_definition",2,$1,$2); $$->right=$3;}
	;

parameter_list: 
	parameter_declaration {$$=new_node_r("parameter_list",$1);}
	| parameter_list "," parameter_declaration {$$=new_node("parameter_list",1,$1); $$->right=$3;}
	;

parameter_declaration: 
	declaration_specifiers direct_declarator {$$=new_node("parameter_declaration",1,$1); $$->right=$2;}
	;

block: 
	"{" "}" {$$=NULL;}
	| "{" blockitem_list "}" {$$=$2;}
	;

blockitem_list: 
    blockitem {$$=new_node_r("blockitem_list",$1);}
    | blockitem_list blockitem {$$=new_node("blockitem_list",1,$1); $$->right=$2;}
    ;

blockitem: 
    statement {$$=$1;}
    | declaration {$$=$1;}
    ;

statement: 
	block {$$=$1;}
	| expression_statement {$$=$1;}
	| selection_statement {$$=$1;}
	| iteration_statement {$$=$1;}
	| jump_statement {$$=$1;}
	;

expression_statement: 
	";" {$$=NULL;}
	| expression ";" {$$=new_node_r("expression_statement",$1);}
	;

expression: 
	assignment_expression {$$=$1;} 
	| expression "," assignment_expression {$$=new_node("expression",3,$1,$2,$3);} //!
	;

assignment_expression: 
	conditional_expression {$$=$1;}
	| unary_expression assignment_operator assignment_expression {$$=$2; $$->left=$1; $$->right=$3;}
	;

conditional_expression: 
	logical_or_expression {$$=$1;}
	;

logical_or_expression: 
	logical_and_expression {$$=$1;}
	| logical_or_expression "||" logical_and_expression {$$=$2; $$->left=$1; $$->right=$3;}
	;

logical_and_expression: 
	equality_expression {$$=$1;}
	| logical_and_expression "&&" equality_expression {$$=$2; $$->left=$1; $$->right=$3;}
	;

equality_expression: 
	relational_expression {$$=$1;}
	| equality_expression "==" relational_expression {$$=$2; $$->left=$1; $$->right=$3;}
	| equality_expression "!=" relational_expression {$$=$2; $$->left=$1; $$->right=$3;}
	;

relational_expression: 
	additive_expression {$$=$1;}
	| relational_expression "<" additive_expression {$$=$2; $$->left=$1; $$->right=$3;}
	| relational_expression ">" additive_expression {$$=$2; $$->left=$1; $$->right=$3;} 
	| relational_expression "<=" additive_expression {$$=$2; $$->left=$1; $$->right=$3;}
	| relational_expression ">=" additive_expression {$$=$2; $$->left=$1; $$->right=$3;}
	;

additive_expression: 
	multiplicative_expression {$$=$1;}
	| additive_expression "+" multiplicative_expression {$$=$2; $$->left=$1; $$->right=$3;}
	| additive_expression "-" multiplicative_expression {$$=$2; $$->left=$1; $$->right=$3;}
	;

multiplicative_expression: 
	unary_expression {$$=$1;}
	| multiplicative_expression "*" unary_expression {$$=$2; $$->left=$1; $$->right=$3;}
	| multiplicative_expression "/" unary_expression {$$=$2; $$->left=$1; $$->right=$3;}
	| multiplicative_expression "%" unary_expression {$$=$2; $$->left=$1; $$->right=$3;}
	;

unary_expression: 
	postfix_expression {$$=$1;}
	| unary_operator unary_expression {$$=$1; $$->right=$2;}
	;

postfix_expression: 
	primary_expression {$$=$1;}
	| postfix_expression "[" expression "]" {$$=new_node("arr_postfix_expression",1,$1); $$->right=$3;}
	| postfix_expression "(" ")" {$$=new_node("func_postfix_expression",1,$1); $$->right=NULL;}
	| postfix_expression "(" argument_expression_list ")" {$$=new_node("func_postfix_expression",1,$1); $$->right=$3;}
    ;

primary_expression: 
	IDENTIFIER {$$=$1;}
	| CONSTANT {$$=$1;}
	| "(" expression ")" {$$=$2;}
	;

argument_expression_list: 
	assignment_expression {$$=new_node_r("argument_expression_list",$1);}
	| argument_expression_list "," assignment_expression {$$=new_node("argument_expression_list",1,$1); $$->right=$3;}
	;

unary_operator: 
	"+" {$$=$1;}
	| "-" {$$=$1;}
	| "!" {$$=$1;}
	;

assignment_operator: 
    "=" {$$=$1;}
    ;

selection_statement: 
	IF "(" expression ")" statement %prec LOWER_THAN_ELSE {$$=new_node("if_statement",1,$3); $$->right=$5;}
	| IF "(" expression ")" statement ELSE statement {$$=new_node("if_statement",1,$3); $$->right=$6; $6->left=$5; $6->right=$7;}
	;

iteration_statement: 
	WHILE "(" expression ")" statement {$$=new_node("while_statement",1,$3); $$->right=$5;}
    ;

jump_statement: 
	CONTINUE ";" {$$=$1;}
	| BREAK ";" {$$=$1;}
	| RETURN ";" {$$=$1;}
	| RETURN expression ";" {$$=$1; $$->right=$2;}
	;

%%

void yyerror(const char*s)
{
	printf("error(line %d): %s at %s \n", yylineno, s, yytext);
}

AST *astRoot;

AST *parseAST(const char* filename)
{
    astRoot = NULL;
	yyparse_init(filename);

    yyparse();

	yyparse_cleanup();
    return astRoot;
}