%option noyywrap nodefault yylineno reentrant bison-bridge
%option header-file="purecalc.lex.h"
%{
#include "purecalc.h"
#include "purecalc.tab.h"
%}

EXP ([Ee][-+]?[0-9]+)

%%
%{
struct pcdata *pp = yyextra;
%}

"+" |
"-" |
"*" |
"/" |
"=" |
"|" |
"," |
";" |
"(" |
")" { return yytext[0]; }

">" { yylval->fn = 1; return CMP; }
"<" { yylval->fn = 2; return CMP; }
"<>" { yylval->fn = 3; return CMP; }
"==" { yylval->fn = 4; return CMP; }
">=" { yylval->fn = 5; return CMP; }
"<=" { yylval->fn = 6; return CMP; }

"if" { return IF; }
"then" { return THEN; }
"else" { return ELSE; }
"while" { return WHILE; }
"do" { return DO; }
"let" { return LET; }

"sqrt" { yylval->fn = B_sqrt; return FUNC; }
"exp" { yylval->fn = B_exp; return FUNC; }
"log" { yylval->fn = B_log; return FUNC; }
"print" { yylval->fn = B_print; return FUNC; }

[a-zA-Z_][a-zA-Z_0-9]* { yylval->s = lookup(pp, yytext); return NAME; }

[0-9]+"."[0-9]*{EXP}? |
"."?[0-9]+{EXP}? { yylval->d = atof(yytext); return NUMBER; }

"//".*
[ \t\r]

\\\n { printf("c> "); }

\n { return EOL; }

. { yyerror(pp, "Mystery character %c\n", *yytext); }

<<EOF>> { exit(0); }

%%
