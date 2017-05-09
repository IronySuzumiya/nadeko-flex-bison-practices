%option noyywrap nodefault yylineno case-insensitive
%{
#include "../obj/pmysql.tab.h"
#include <stdarg.h>
#include <string.h>

void yyerror(char *s, ...);

int oldstate;
%}

%x COMMENT
%x BTWMODE

%%

ADD { return ADD; }
ALL { return ALL; }
ALTER { return ALTER; }
ANALYZE { return ANALYZE; }
