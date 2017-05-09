/*
 *      A C-Like Language Lexical Highlighter
 *
 *      Platform:       Linux (Ubuntu 16.04 tests pass)
 *      Author:         Sangoku Nadeko
 *      Contact:        811073137@qq.com
 */
 
%option noyywrap yylineno

dig                     [0-9]
oct                     [0-7]
hex                     [0-9a-fA-F]
letter                  [_A-Za-z]
exp                     [Ee][-+]?[0-9]+

number                  [-+]?(0(x|X){hex}+)|(0{oct}+)|{dig}*("."{dig}+)?{exp}?
charac                  \'(.|\\.)\'
string                  \"(\\?.)*?\"
boolval                 true|false
constant	            {number}|{charac}|{string}|{boolval}

identifier              {letter}({letter}|{dig})*

macro                   ^[ \t]*#.*

operator	            ([~!%^&|*\-+=<>/()?:.]|"<="|">="|"=="|"!="|"++"|"--"|"<<"|">>"|"<<="|">>="|"+="|"-="|"*="|"/="|"%="|"&="|"|="|"^="|"&&"|"||"|"sizeof")

delimiter		        [\{\}\[\]\;\,]

comment	                "//".*

escape                  "\\"
varargs                 "..."
extra                   {escape}|{varargs}

%x BLOCKCOMMENT

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NONE                    "\e[0m"
#define BLACK                   "\e[0;30m"
#define L_BLACK                 "\e[1;30m"
#define RED                     "\e[0;31m"
#define L_RED                   "\e[1;31m"
#define GREEN                   "\e[0;32m"
#define L_GREEN                 "\e[1;32m"
#define BROWN                   "\e[0;33m"
#define YELLOW                  "\e[1;33m"
#define BLUE                    "\e[0;34m"
#define L_BLUE                  "\e[1;34m"
#define PURPLE                  "\e[0;35m"
#define L_PURPLE                "\e[1;35m"
#define CYAN                    "\e[0;36m"
#define L_CYAN                  "\e[1;36m"
#define GRAY                    "\e[0;37m"
#define WHITE                   "\e[1;37m"

#define BOLD                    "\e[1m"
#define UNDERLINE               "\e[4m"
#define BLINK                   "\e[5m"
#define REVERSE                 "\e[7m"
#define HIDE                    "\e[8m"
#define CLEAR                   "\e[2J"
#define CLRLINE                 "\r\e[K" //or "\e[1K\r"

#define MACRO_COLOR             BROWN
#define COMMENT_COLOR           GREEN
#define CONSTANT_COLOR          PURPLE
#define OPERATOR_COLOR          GRAY
#define RESERVED_WORD_COLOR     BLUE
#define IDENTIFIER_COLOR        L_CYAN
#define INVALID_SYMBOL_COLOR    RED

#define LINE_NO_COLOR           L_RED

#define DYES(string, color)     color string NONE

#define RESERVED_WORDS_NUM      34

static const char *reserved_words[RESERVED_WORDS_NUM] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if", "int",
    "long", "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void", "volatile",
    "while", "inline", "bool",
};

//static unsigned char reserved_words_hash[RESERVED_WORDS_NUM];

static int get_reserved_word_id(char* word);

int delimiter_num = 0;
int reserved_word_num = 0;
int operator_num = 0;
int constant_num = 0;
int identifier_num = 0;
int macro_num = 0;
int comment_num = 0;
int extra_num = 0;
int invalid_symbol_num = 0;

FILE *statistic_output;
%}

%%

{comment}	{
    printf(DYES("%s", COMMENT_COLOR), yytext);
	fprintf(statistic_output,     "[%4d]comment:            %s\n", yylineno, yytext);
	comment_num++;
}

"/*" {
	BEGIN BLOCKCOMMENT;
}

<BLOCKCOMMENT>"*/" {
    comment_num++;
	BEGIN 0;
}

<BLOCKCOMMENT>. { }

<BLOCKCOMMENT>"\n" { }

{macro}	{
	printf(DYES("%s", MACRO_COLOR), yytext);
	fprintf(statistic_output,     "[%4d]macro:              %s\n", yylineno, yytext);
	macro_num++;
}

{constant}	{
	printf(DYES("%s", CONSTANT_COLOR), yytext);
	fprintf(statistic_output,     "[%4d]constant:           %s\n", yylineno, yytext);
	constant_num++;
}

{operator}	{
	int id = 0;
	while(yyleng > 0) {
		id += yytext[--yyleng];
	}
	printf(DYES("%s", OPERATOR_COLOR), yytext);
	fprintf(statistic_output,     "[%4d]operator(%2d):       %s\n", yylineno, id, yytext);
	operator_num++;
}

{identifier}	{
	int id;
	if ((id = get_reserved_word_id(yytext)) >= 0) {
		printf(DYES("%s", RESERVED_WORD_COLOR), yytext);
		fprintf(statistic_output, "[%4d]reserved word(%2d):  %s\n", yylineno, id, yytext);
		reserved_word_num++;
	} else {
		printf(DYES("%s", IDENTIFIER_COLOR), yytext);
		fprintf(statistic_output, "[%4d]identifier:         %s\n", yylineno, yytext);
		identifier_num++;
	}
}

{delimiter}	{
	printf("%s", yytext);
	fprintf(statistic_output,     "[%4d]delimiter:          %s\n", yylineno, yytext);
	delimiter_num++;
}

{extra} {
    printf("%s", yytext);
    fprintf(statistic_output,     "[%4d]extra symbol:       %s\n", yylineno, yytext);
    extra_num++;
}

[^ \t\r\n]	{
	printf(DYES("%s", INVALID_SYMBOL_COLOR), yytext);
	fprintf(statistic_output,     "[%4d]invalid symbol:     %s\n", yylineno, yytext);
    invalid_symbol_num++;
}

\n {
    printf("\n");
    printf(DYES("%4d: ", LINE_NO_COLOR), yylineno);
}

%%

int main(int argc, char **argv) {
    int i;
    
    /*for(int i = 0; i < RESERVED_WORDS_NUM; ++i) {
        char *sym = reserved_words[i];
        char c;
        while(c = *sym++) reserved_words_hash[i] = reserved_words_hash[i] * 9 ^ c;
        printf("%s: %u\n", reserved_words[i], reserved_words_hash[i]);
    }*/
    
    statistic_output = fopen("output.txt", "w");
    
    if(argc < 2) {
        yylineno = 1;
        printf(DYES("%4d: ", LINE_NO_COLOR), yylineno);
        yylex();
    } else {
        for(i = 1; i < argc; ++i) {
            FILE *f = fopen(argv[i], "r");
            
            if(!f) {
                perror(argv[i]);
                continue;
            }
            
            fprintf(statistic_output, "------     %s     ------\n\n", argv[i]);
            printf("------     %s     ------\n\n", argv[i]);
            
            yyrestart(f);
            yylineno = 1;
            printf(DYES("%4d: ", LINE_NO_COLOR), yylineno);
            
            yylex();
            
            fprintf(statistic_output, "\n\n");
            printf("\n\n");
            
            fclose(f);
        }
    }
    
    fclose(statistic_output);

	printf( "\n\n-----   Summary  -----\n");
	printf(DYES("macros:             %d\n", MACRO_COLOR),           macro_num);
	printf(DYES("reserved words:     %d\n", RESERVED_WORD_COLOR),   reserved_word_num);
	printf(DYES("operators:          %d\n", OPERATOR_COLOR),        operator_num);
	printf(DYES("constants:          %d\n", CONSTANT_COLOR),        constant_num);
	printf(DYES("identifiers:        %d\n", IDENTIFIER_COLOR),      identifier_num);
	printf(DYES("comments:           %d\n", COMMENT_COLOR),         comment_num);
    printf(     "delimiters:         %d\n",                         delimiter_num);
    printf(     "extra symbols:      %d\n",                         extra_num);
    printf(DYES("invalid symbols:    %d\n", INVALID_SYMBOL_COLOR),  invalid_symbol_num);
    
	return 0;
}

int get_reserved_word_id(char *word) {
    for(int i = 0; i < RESERVED_WORDS_NUM; ++i) {
        if(!strcmp(word, reserved_words[i])) {
            return i;
        }
    }
    return -1;
}
