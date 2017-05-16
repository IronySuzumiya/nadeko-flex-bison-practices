%{
#include <stdio.h>
#include <stdlib.h>
#include "scc.h"

extern int yylineno;

int yylex(void);

struct locsym *yyloctab;
%}

%union {
    struct ast *yya;
    struct sym *yys;
    struct symlist *yyslist;
    struct arglist *yyargs;
    struct funcdef *yyfinfo;
    float fv;
    int iv;
    char cv;
    char *sv;
}

%token <fv> FLOAT
%token <iv> INT
%token <cv> CHAR
%token <sv> STRING
%token <yys> ID

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
%type <yya> param_list
%type <yya> exp block
%type <yya> stmt stmt_list
%type <yyargs> _arg_list arg_list
%type <yyfinfo> func_info func_implheader
%type <yya> func_declr func_impl
%type <yys> var
%type <yyslist> _var_list var_list
%type <yya> glo_var_declr
%type <yya> glo_declr glo_declr_list

%start program

%%

dtype
: TYPE_CHAR { $$ = NODETYPE_CHAR; }
| TYPE_INT { $$ = NODETYPE_INT; }
| TYPE_FLOAT { $$ = NODETYPE_FLOAT; }

param_list
: exp { $$ = $1; }
| exp ',' param_list { if($3 == NULL) $$ = $1; else $$ = make_ast(NODETYPE_LIST, $1, $3); }
| %empty { $$ = NULL; }
;

exp
: ID '=' exp { $$ = make_symasgn(yyloctab, $1, $3); }
| ID { $$ = make_symref(yyloctab, $1); }
| INT { $$ = make_intval($1); }
| CHAR { $$ = make_charval($1); }
| FLOAT { $$ = make_floatval($1); }
| STRING { $$ = make_stringval($1); }
| exp '+' exp { $$ = make_ast('+', $1, $3); }
| exp '-' exp { $$ = make_ast('-', $1, $3); }
| exp '*' exp { $$ = make_ast('*', $1, $3); }
| exp '/' exp { $$ = make_ast('/', $1, $3); }
| exp '%' exp { $$ = make_ast('%', $1, $3); }
| exp SHL exp { $$ = make_ast(NODETYPE_SHL, $1, $3); }
| exp SHR exp { $$ = make_ast(NODETYPE_SHR, $1, $3); }
| exp '<' exp { $$ = make_ast('<', $1, $3); }
| exp '>' exp { $$ = make_ast('>', $1, $3); }
| exp LE exp { $$ = make_ast(NODETYPE_LE, $1, $3); }
| exp GE exp { $$ = make_ast(NODETYPE_GE, $1, $3); }
| exp EQ exp { $$ = make_ast(NODETYPE_EQ, $1, $3); }
| exp NE exp { $$ = make_ast(NODETYPE_NE, $1, $3); }
| exp '&' exp { $$ = make_ast('&', $1, $3); }
| exp '^' exp { $$ = make_ast('^', $1, $3); }
| exp '|' exp { $$ = make_ast('|', $1, $3); }
| exp LAND exp { $$ = make_ast(NODETYPE_LAND, $1, $3); }
| exp LOR exp { $$ = make_ast(NODETYPE_LOR, $1, $3); }
| '(' exp ')' { $$ = $2; }
| '-' exp %prec UMINUS { $$ = make_ast(NODETYPE_NEGATIVE, $2, NULL); }
| '+' exp %prec UMINUS { $$ = make_ast(NODETYPE_POSITIVE, $2, NULL); }
| '~' exp { $$ = make_ast('~', $2, NULL); }
| '!' exp { $$ = make_ast('!', $2, NULL); }
| INC ID { $$ = make_ast(NODETYPE_PREINC, make_symref(yyloctab, $2), NULL); }
| DEC ID { $$ = make_ast(NODETYPE_PREDEC, make_symref(yyloctab, $2), NULL); }
| ID INC { $$ = make_ast(NODETYPE_POSTINC, make_symref(yyloctab, $1), NULL); }
| ID DEC { $$ = make_ast(NODETYPE_POSTDEC, make_symref(yyloctab, $1), NULL); }
| SIZEOF ID { $$ = make_ast(NODETYPE_SIZEOF, make_symref(yyloctab, $2), NULL); }
| ID '(' param_list ')' { $$ = make_funccall($1, $3); }
;

block
: IF '(' exp ')' '{' stmt_list '}' { $$ = make_flow(NODETYPE_IF, $3, $6, NULL); }
| IF '(' exp ')' '{' stmt_list '}' ELSE '{' stmt_list '}' { $$ = make_flow(NODETYPE_IF, $3, $6, $10); }
| WHILE '(' exp ')' '{' stmt_list '}' { $$ = make_flow(NODETYPE_WHILE, $3, $6, NULL); }
;

stmt
: exp ';' { $$ = $1; }
| block { $$ = $1; }
| BREAK ';' { $$ = make_ast(NODETYPE_BREAK, NULL, NULL); }
| CONTINUE ';' { $$ = make_ast(NODETYPE_CONTINUE, NULL, NULL); }
| RETURN exp ';' { $$ = make_ast(NODETYPE_RETURN, $2, NULL); }
| dtype var_list ';' { $$ = make_locvardef(yyloctab, $1, $2); }
;

stmt_list
: stmt stmt_list { if($2 = NULL) $$ = $1; else $$ = make_ast(NODETYPE_LIST, $1, $2); }
| %empty { $$ = NULL; }
;

_arg_list
: ',' dtype ID _arg_list { $$ = make_arglist(make_typelist($2, $4->tl), make_symlist($3, $4->sl)); free($4); }
| %empty { $$ = make_arglist(NULL, NULL); }
;

arg_list
: dtype ID _arg_list { $$ = make_arglist(make_typelist($1, $3->tl), make_symlist($2, $3->sl)); free($3); }
| TYPE_VOID { $$ = make_arglist(NULL, NULL); }
;

func_info
: TYPE_VOID ID '(' arg_list ')' { $$ = make_funcinfo(NODETYPE_VOID, $2, $4); }
| dtype ID '(' arg_list ')' { $$ = make_funcinfo($1, $2, $4); }
;

func_implheader
: func_info '{' { $$ = make_funcimplheader($1); }
;

func_impl
: func_implheader stmt_list '}' { $$ = make_funcimpl($1, $2); }
;

func_declr
: func_info ';' { $$ = make_funcdeclr($1, 0); }
;

var
: ID { $$ = $1; }
;

_var_list
: ',' var _var_list { $$ = make_symlist($2, $3); }
| %empty { $$ = NULL; }
;

var_list
: var _var_list { $$ = make_symlist($1, $2); }
;

glo_var_declr
: dtype var_list ';' { $$ = make_glovardef($1, $2); }
;

glo_declr
: glo_var_declr { $$ = $1; }
| func_declr { $$ = $1; }
| func_impl { $$ = $1; }
;

glo_declr_list
: glo_declr glo_declr_list { if($2 == NULL) $$ = $1; else $$ = make_ast(NODETYPE_LIST, $1, $2); }
| %empty { $$ = NULL; }
;

program
: glo_declr_list { }
;

%%

int main() {
	return yyparse();
}
