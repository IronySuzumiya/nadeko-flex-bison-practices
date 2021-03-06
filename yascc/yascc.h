#ifndef __YASCC_H
#define __YASCC_H

/* * * * * * * * * * * * * * * * * * * * * *
 *                                          *
 *    Yet Another Simple C-like Compiler    *
 *                          --YASCC         *
 *                                          *
 *          Author:  Sangoku Nadeko         *
 *          Email:   81103137@qq.com        *
 *          Github:  IronySuzumiya          *
 *          Date:    2017-5-15              *
 *                                          *
  * * * * * * * * * * * * * * * * * * * * * */




#define DEBUG

/* data structures */

struct ast {
    int nodetype;
    struct ast *l;
    struct ast *r;
};

struct sym {
    char *name;
};

struct glosym {
    char *name;
    int nodetype;
    void *prop; // glovar or func
};

struct glovar {
    struct ast *val;
};

struct locsym {
    char *name;
    struct ast *val;
};

struct symlist {
    struct sym *s;
    struct symlist *next;
};

struct typelist {
    int datatype;
    struct typelist *next;
};

struct arglist {
    struct typelist *tl;
    struct symlist *sl;
};

struct func {
    int retntype;
    int nargs;
    struct arglist *args;
    struct locsym *loctab;
    struct ast *body;
};

struct symref {
    int nodetype;
    struct sym *s;
};

struct symasgn {
    int nodetype;
    struct symref *sr;
    struct ast *val;
};

struct funccall {
    int nodetype;
    struct glosym *f;
    struct ast *args;
};

struct flow {
    int nodetype; // 'I' for if, and 'W' for while
    struct ast *cond;
    struct ast *tt; // true then
    struct ast *ft; // false then
};

struct intval {
    int datatype;
    int val;
};

struct charval {
    int datatype;
    char val;
};

struct floatval {
    int datatype;
    float val;
};

struct stringval {
    int datatype;
    char *val;
};

struct vardef {
    int nodetype;
    int datatype;
    struct symlist *vlist;
};

struct funcdef {
    int nodetype;
    int retntype;
    struct sym *f;
    struct arglist *args;
};




/* node-type code */

#define NODETYPE_INT            'i'

#define NODETYPE_CHAR           'c'

#define NODETYPE_FLOAT          'f'

#define NODETYPE_STRING         's'

#define NODETYPE_VOID           'v'

#define NODETYPE_LIST           'L'

#define NODETYPE_SHL            '0'

#define NODETYPE_SHR            '1'

#define NODETYPE_LE             '2'

#define NODETYPE_GE             '3'

#define NODETYPE_EQ             '4'

#define NODETYPE_NE             '5'

#define NODETYPE_LAND           '6'

#define NODETYPE_LOR            '7'

#define NODETYPE_NEGATIVE       '8'

#define NODETYPE_POSITIVE       '9'

#define NODETYPE_PREINC         1

#define NODETYPE_PREDEC         2

#define NODETYPE_POSTINC        3

#define NODETYPE_POSTDEC        4

#define NODETYPE_SIZEOF         5

#define NODETYPE_IF             'I'

#define NODETYPE_WHILE          'W'

#define NODETYPE_BREAK          'B'

#define NODETYPE_CONTINUE       'C'

#define NODETYPE_RETURN         'R'

#define NODETYPE_VAR            'V'

#define NODETYPE_FUNCDECLR      'D'

#define NODETYPE_FUNCIMPL       'M'

#define NODETYPE_FUNCCALL       'F'

#define NODETYPE_GLOREF         'g'

#define NODETYPE_LOCREF         'l' // L

#define NODETYPE_SYMASGN        '='

#define NODETYPE_GLOVARDEF      6

#define NODETYPE_LOCVARDEF      7

#define NODETYPE_FUNCDECLRAST   8

#define NODETYPE_FUNCIMPLAST    9




/* symbol table */

#define NAMEHASH        9997

struct sym symtab[NAMEHASH];

struct sym *store_sym(char *name);


#define GLOSYMTABSIZE   4096

struct glosym glosymtab[GLOSYMTABSIZE];

struct glosym *register_glosym(char *name);

struct glosym *lookup_glosym(char *name);


#define LOCSYMTABSIZE   1024

//struct locsym locsymtab[LOCSYMTABSIZE]; // dynamic allocate

struct locsym *register_locsym(struct locsym tab[], char *name);

struct locsym *lookup_locsym(struct locsym tab[], char *name);




/* node-making functions */

struct ast *make_ast(int nodetype, struct ast *l, struct ast *r);

struct ast *make_funccall(struct sym *s, struct ast *args);

struct ast *make_symref(struct locsym tab[], struct sym *s);

struct ast *make_symasgn(struct locsym tab[], struct sym *s, struct ast *val);

struct ast *make_flow(int nodetype, struct ast *cond, struct ast *tt, struct ast *ft);

struct ast *make_intval(int val);

struct ast *make_charval(char val);

struct ast *make_floatval(float val);

struct ast *make_stringval(char *val);

struct symlist *make_symlist(struct sym *s, struct symlist *next);

struct typelist *make_typelist(int datatype, struct typelist *next);

struct arglist *make_arglist(struct typelist *tl, struct symlist *sl);

struct ast *make_glovardef(int datatype, struct symlist *vlist);

struct ast *make_locvardef(struct locsym tab[], int datatype, struct symlist *vlist);

struct funcdef *make_funcinfo(int retntype, struct sym *f, struct arglist *args);

struct ast *make_funcdeclr(struct funcdef *finfo, char impl);

struct funcdef *make_funcimplheader(struct funcdef *finfo);

struct ast *make_funcimpl(struct funcdef *finfo, struct ast *body);




/* other functions */

#define STACK_SIZE 512

void eval(struct ast *);

void traverse_ast(struct ast *);

void free_ast(struct ast *);

void free_symlist(struct symlist *);

void free_typelist(struct typelist *);

void free_arglist(struct arglist *);

void yyerror(char *s, ...);




#endif
