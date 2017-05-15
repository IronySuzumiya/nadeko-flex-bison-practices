%define api.pure
%parse-param { struct pcdata *pp }

%{
#include <stdio.h>
#include <stdlib.h>
%}

%union {
    struct ast *a;
    double d;
    struct symbol *s;
    struct symlist *sl;
    int fn;
}

%{
#include "purecalc.lex.h"
#include "purecalc.h"
#define YYLEX_PARAM pp->scaninfo
%}

%token <d> NUMBER
%token <s> NAME
%token <fn> FUNC
%token EOL

%token IF THEN ELSE WHILE DO LET

%nonassoc <fn> CMP
%right '='
%left '+' '-'
%left '*' '/'
%nonassoc '|' UMINUS

%type <a> exp stmt list explist
%type <sl> symlist

%start calc

%%

calc: EOL { pp->ast = NULL; YYACCEPT; }
| stmt EOL { pp->ast = $1; YYACCEPT; }
| LET NAME '(' symlist ')' '=' list EOL {
    dodef(pp, $2, $4, $7);
    printf("%d: Defined %s\n", yyget_lineno(pp->scaninfo), $2->name);
    pp->ast = NULL;
    YYACCEPT;
}
;

stmt: IF exp THEN list { $$ = newflow(pp, 'I', $2, $4, NULL); }
| IF exp THEN list ELSE list { $$ = newflow(pp, 'I', $2, $4, $6); }
| WHILE exp DO list { $$ = newflow(pp, 'W', $2, $4, NULL); }
| exp
;

list: { $$ = NULL; }
| stmt ';' list {
    if($3 == NULL)
        $$ = $1;
    else
        $$ = newast(pp, 'L', $1, $3);
}
;

exp: exp CMP exp { $$ = newcmp(pp, $2, $1, $3); }
| exp '+' exp { $$ = newast(pp, '+', $1, $3); }
| exp '-' exp { $$ = newast(pp, '-', $1, $3); }
| exp '*' exp { $$ = newast(pp, '*', $1, $3); }
| exp '/' exp { $$ = newast(pp, '/', $1, $3); }
| '|' exp { $$ = newast(pp, '|', $2, NULL); }
| '(' exp ')' { $$ = $2; }
| '-' exp %prec UMINUS { $$ = newast(pp, 'M', $2, NULL); }
| NUMBER { $$ = newnum(pp, $1); }
| NAME { $$ = newref(pp, $1); }
| NAME '=' exp { $$ = newasgn(pp, $1, $3); }
| FUNC '(' explist ')' { $$ = newfunc(pp, $1, $3); }
| NAME '(' explist ')' { $$ = newcall(pp, $1, $3); }
;

explist: exp
| exp ',' explist { $$ = newast(pp, 'L', $1, $3); }
;

symlist: NAME { $$ = newsymlist(pp, $1, NULL); }
| NAME ',' symlist { $$ = newsymlist(pp, $1, $3); }
;

%%
