%{
#include <stdio.h>
#include <stdlib.h>
#include "scc.h"

extern int yylineno;

int yylex(void);

struct locsym *yyloctab;
%}

%union {
    struct ast *a;
    struct sym *s;
    struct symlist *slist;
    float fv;
    int iv;
    char cv;
    char *sv;
}

%token <fv> FLOAT
%token <iv> INT
%token <cv> CHAR
%token <sv> STRING
%token <s> ID

%token LOR LAND EQ NE LE GE
%token SHL SHR INC DEC SIZEOF

%token TYPE_VOID TYPE_INT TYPE_CHAR TYPE_FLOAT
%token IF ELSE WHILE BREAK CONTINUE RETURN

%right '='
%left LOR
%left LAND
%left '|'
%left '^'
%left '&'
%left EQ NE
%left '<' LE '>' GE
%left SHL SHR
%left '+' '-'
%left '*' '/' '%'
%nonassoc UMINUS '~' '!' INC DEC SIZEOF

%type <iv> dtype
%type <a> param_list
%type <a> exp block
%type <a> stmt stmt_list
%type <a> _arg_list arg_list
%type <a> func_declr func_impl
%type <s> var
%type <slist> var_list
%type <a> glo_var_declr
%type <a> glo_declr glo_declr_list

%start program

%%

dtype
: TYPE_CHAR { $$ = 'c'; }
| TYPE_INT { $$ = 'i'; }
| TYPE_FLOAT { $$ = 'f'; }

param_list
: exp { $$ = $1; }
| exp ',' param_list { if($3 == NULL) $$ = $1; else $$ = make_ast('L', $1, $3); }
| %empty { $$ = NULL; }
;

exp
: ID '=' exp { $$ = make_symasgn((struct sym *)$1, $3); }
| ID { $$ = (struct ast *)$1; }
| INT { $$ = make_intval($1); }
| CHAR { $$ = make_charval($1); }
| FLOAT { $$ = make_floatval($1); }
| STRING { $$ = make_stringval($1); }
| exp '+' exp { $$ = make_ast('+', $1, $3); }
| exp '-' exp { $$ = make_ast('-', $1, $3); }
| exp '*' exp { $$ = make_ast('*', $1, $3); }
| exp '/' exp { $$ = make_ast('/', $1, $3); }
| exp '%' exp { $$ = make_ast('%', $1, $3); }
| exp SHL exp { $$ = make_ast('1', $1, $3); }
| exp SHR exp { $$ = make_ast('2', $1, $3); }
| exp '<' exp { $$ = make_ast('<', $1, $3); }
| exp '>' exp { $$ = make_ast('>', $1, $3); }
| exp LE exp { $$ = make_ast('3', $1, $3); }
| exp GE exp { $$ = make_ast('4', $1, $3); }
| exp EQ exp { $$ = make_ast('5', $1, $3); }
| exp NE exp { $$ = make_ast('6', $1, $3); }
| exp '&' exp { $$ = make_ast('&', $1, $3); }
| exp '^' exp { $$ = make_ast('^', $1, $3); }
| exp '|' exp { $$ = make_ast('|', $1, $3); }
| exp LAND exp { $$ = make_ast('7', $1, $3); }
| exp LOR exp { $$ = make_ast('8', $1, $3); }
| '(' exp ')' { $$ = $2; }
| '-' exp %prec UMINUS { $$ = make_ast('n', $2, NULL); }
| '+' exp %prec UMINUS { $$ = make_ast('p', $2, NULL); }
| '~' exp { $$ = make_ast('~', $2, NULL); }
| '!' exp { $$ = make_ast('!', $2, NULL); }
| INC ID { $$ = make_ast('9', (struct ast *)$2, NULL); }
| DEC ID { $$ = make_ast('0', (struct ast *)$2, NULL); }
| ID INC { $$ = make_ast(128 + '1', (struct ast *)$1, NULL); }
| ID DEC { $$ = make_ast(128 + '2', (struct ast *)$1, NULL); }
| SIZEOF ID { $$ = make_ast(128 + '3', (struct ast *)$2, NULL); }
| ID '(' param_list ')' { $$ = make_funccall((struct sym *)$1, $3); }
;

block
: IF '(' exp ')' '{' stmt_list '}' { $$ = make_flow('I', $3, $6, NULL); }
| IF '(' exp ')' '{' stmt_list '}' ELSE '{' stmt_list '}' { $$ = make_flow('I', $3, $6, $10); }
| WHILE '(' exp ')' '{' stmt_list '}' { $$ = make_flow('W', $3, $6, NULL); }
;

stmt
: exp ';' { $$ = $1; }
| block { $$ = $1; }
| BREAK ';' { $$ = make_ast('B', NULL, NULL); }
| CONTINUE ';' { $$ = make_ast('C', NULL, NULL); }
| RETURN exp ';' { $$ = make_ast('R', $2, NULL); }
| dtype var_list ';' { $$ = make_locvardef($2, $1); }
;

stmt_list
: stmt stmt_list { if($2 = NULL) $$ = $1; else $$ = make_ast('L', $1, $2); }
| %empty { $$ = NULL; }
;

_arg_list
: ',' dtype ID _arg_list { $$ = make_funcargs((struct sym *)make_funcarg($3, $2), (struct symlist *)$4); }
| %empty { $$ = NULL; }
;

arg_list
: dtype ID _arg_list { $$ = make_funcargs((struct sym *)make_funcarg($2, $1), (struct symlist *)$3); }
| TYPE_VOID { $$ = NULL; }
;

func_impl
: TYPE_VOID ID '(' arg_list ')' '{' stmt_list '}' { $$ = def_func((struct sym *)$1, $3); }
| dtype ID '(' arg_list ')' '{' stmt_list '}' { $$ = def_func((struct sym *)$1, $3); }
;

func_declr
: TYPE_VOID ID '(' arg_list ')' ';' { $$ = $1; }
| dtype ID '(' arg_list ')' '{' stmt_list '}' { $$ = def_func((struct sym *)$1, $3); }
;

var
: ID { $$ = $1; }
;

var_list
: var { $$ = make_symlist($1, NULL); }
| var ',' var_list { $$ = make_symlist($1, $3); }
| %empty { $$ = NULL; }
;

glo_var_declr
: dtype var_list ';' { $$ = make_glovardef($2, $1); }
;

glo_declr
: glo_var_declr { $$ = $1; }
| func_declr { $$ = $1; }
| func_impl { $$ = $1; }
;

glo_declr_list
: glo_declr glo_declr_list { if($2 == NULL) $$ = $1; else $$ = make_ast('L', $1, $2); }
| %empty { $$ = NULL; }
;

program
: glo_declr_list { }
;

%%

int main() {
	return yyparse();
}
