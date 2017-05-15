%option noyywrap nodefault yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*"/*".*?"*//*" {
    int i;
    for(i = 2; i < strlen(yytext) - 2; ++i) {
        if(yytext[i] == '*' && yytext[i+1] == '/') {
            yyless(i + 2);
            break;
        }
    }
    printf("match: ");
    char *str = malloc(i-1);
    strncpy(str, &yytext[2], i-2);
    puts(str);
}*/
%}

%x COMMENT

%%

"//".*$ { printf("match\n"); }

"/*" { BEGIN COMMENT; }

<COMMENT>\n { }

<COMMENT>"*/" { BEGIN 0; }

<COMMENT>. { }

a.*?z {
    int i;
    printf("match: ");
    putchar(yytext[0]);
    for(i = 1; i < strlen(yytext); ++i) {
        if(yytext[i] == 'z') {
            putchar(yytext[i]);
            yyless(i + 1);
            break;
        } else {
            putchar(yytext[i]);
        }
    }
}

\n { putchar('\n'); }

. { putchar(yytext[0]); }

%%

int main()
{
    yylex();
    printf("yylineno = %d\n", yylineno);
    return 0;
}
