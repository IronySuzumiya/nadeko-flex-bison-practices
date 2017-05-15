%option noyywrap nodefault yylineno
%{
#include <stdlib.h>
#include <string.h>
#include "scc.tab.h"
#include "scc.h"
%}

hexnumber               0x[0-9a-fA-F]+
octnumber               0[0-7]+
dignumber               0|([1-9][0-9]*)
floatnumber             {dignumber}?\.{dignumber}

charac                  \'\\?.\'

string                  \"(\\?.)*\"

identifier              [_A-Za-z][_A-Za-z0-9]*

macro                   ^[ \t]*#.*

delimiter		        [\{\}\[\]\;\,]

comment	                "//".*$

%x MULTILINE_COMMENT

%%

{dignumber} {
    yylval.iv = strtol(yytext, NULL, 10);
    return INT;
}

{hexnumber} {
    yylval.iv = strtol(yytext, NULL, 16);
    return INT;
}

{octnumber} {
    yylval.iv = strtol(yytext, NULL, 8);
    return INT;
}

{floatnumber} {
    yylval.fv = strtod(yytext, NULL);
    return FLOAT;
}

{charac} {
    if(yytext[1] == '\\') {
        switch(yytext[2]) {
            case '\'': yylval.cv = '\''; break;
            case '\\': yylval.cv = '\\'; break;
            case 'n': yylval.cv = '\n'; break;
            case 'r': yylval.cv = '\r'; break;
            case 't': yylval.cv = '\t'; break;
            default: yyerror("Mystery escape char %c at line %d\n", yytext[2], yylineno);
        }
    } else {
        yylval.cv = yytext[1];
    }
    return CHAR;
}

{string} {
    int i;
    for(i = 1; i < strlen(yytext) - 1; ++i) {
        if(yytext[i] == '\\') {
            ++i;
            continue;
        } else if(yytext[i] == '"') {
            yyless(i + 1);
            break;
        }
    }
    char *str = malloc(i);
    strncpy(str, &yytext[1], i - 1);
    yylval.sv = str;
    return STRING;
}

"void" { return TYPE_VOID; }
"int" { return TYPE_INT; }
"char" { return TYPE_CHAR; }
"float" { return TYPE_FLOAT; }
"if" { return IF; }
"else" { return ELSE; }
"while" { return WHILE; }
"break" { return BREAK; }
"continue" { return CONTINUE; }
"return" { return RETURN; }

{macro} { }

[~!^&|\-+*/%=<>()\[\],;] {
    return yytext[0];
}

"<=" { return LE; }
">=" { return GE; }
"==" { return EQ; }
"!=" { return NE; }
"++" { return INC; }
"--" { return DEC; }
"<<" { return SHL; }
">>" { return SHR; }
"&&" { return LAND; }
"||" { return LOR; }
"sizeof" { return SIZEOF; }

{identifier} {
    yylval.s = store_sym(yytext);
    return ID;
}

{delimiter} { return yytext[0]; }

{comment} { }

"/*" {
    BEGIN MULTILINE_COMMENT;
}

<MULTILINE_COMMENT>\n { }

<MULTILINE_COMMENT>"*/" { BEGIN 0; }

<MULTILINE_COMMENT>. { }

[ \t\r] { }

\n { }

. { yyerror("Mystery character %c at line %d\n", yytext[0], yylineno); }

%%
