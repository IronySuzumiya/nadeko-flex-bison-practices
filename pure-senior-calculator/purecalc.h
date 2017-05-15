#ifndef __PURE_SENIOR_CALCULATOR_H
#define __PURE_SENIOR_CALCULATOR_H

struct pcdata {
    yyscan_t scaninfo;
    struct symbol *symtab;
    struct ast *ast;
};

struct symbol {
    char *name;
    double value;
    struct ast *func;
    struct symlist *syms;
};

#define NHASH 9997

struct symbol symtab[NHASH];

struct symbol *lookup(struct pcdata *, char *);

struct symlist {
    struct symbol *sym;
    struct symlist *next;
};

struct symlist *newsymlist(struct pcdata *,
                    struct symbol *sym, struct symlist *next);

void symlistfree(struct pcdata *, struct symlist *sl);

enum bifs {
    B_sqrt = 1,
    B_exp,
    B_log,
    B_print
};

struct ast {
    int nodetype;
    struct ast *l;
    struct ast *r;
};

struct fncall {
    int nodetype;
    struct ast *l;
    enum bifs functype;
};

struct ufncall {
    int nodetype;
    struct ast *l;
    struct symbol *s;
};

struct flow {
    int nodetype; // 'I' for if, and 'W' for while
    struct ast *cond;
    struct ast *tl; // then (for if), do (for while)
    struct ast *el; // else (for if, optional)
};

struct numval {
    int nodetype;
    double number;
};

struct symref {
    int nodetype;
    struct symbol *s;
};

struct symasgn {
    int nodetype;
    struct symbol *s;
    struct ast *v;
};

struct ast *newast(struct pcdata *, int nodetype,
                    struct ast *l, struct ast *r);
struct ast *newcmp(struct pcdata *, int cmptype,
                    struct ast *l, struct ast *r);
struct ast *newfunc(struct pcdata *, int functype, struct ast *l);
struct ast *newcall(struct pcdata *, struct symbol *s, struct ast *l);
struct ast *newref(struct pcdata *, struct symbol *s);
struct ast *newasgn(struct pcdata *, struct symbol *s, struct ast *v);
struct ast *newnum(struct pcdata *, double d);
struct ast *newflow(struct pcdata *, int nodetype, struct ast *cond,
                    struct ast *tl, struct ast *tr);

void dodef(struct pcdata *, struct symbol *name,
                    struct symlist *syms, struct ast *stmts);

double eval(struct pcdata *, struct ast *);

void treefree(struct pcdata *, struct ast *);

extern int yylineno;

void yyerror(struct pcdata *, char *s, ...);

#endif
