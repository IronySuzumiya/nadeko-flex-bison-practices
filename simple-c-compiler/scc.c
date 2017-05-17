#include "scc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef DEBUG

static inline void debug_log(char *s, ...) {
    va_list ap;
    va_start(ap, s);
    
    fprintf(stdout, ">> ");
    vfprintf(stdout, s, ap);
    fprintf(stdout, "\n");
}

#define DATATYPE_NAME(a)    ((a) == NODETYPE_CHAR ? "char" : (a) == NODETYPE_INT ? "int" :\
                             (a) == NODETYPE_FLOAT ? "float" : (a) == NODETYPE_VOID ? "void" : "unknown")
#else
#define debug_log(...);
#endif

extern int yylineno;

extern struct locsym *yyloctab;

static void *makesure_malloc(unsigned int m_size) {
    void *m = malloc(m_size);
    if(!m && m_size) {
        yyerror("memory exhausted at line %d\n", yylineno);
        abort();
    }
    return m;
}

static unsigned symhash(char *name) {
    unsigned int hash = 0;
    unsigned c;
    
    while(c = *name++) hash = hash * 9 ^ c;
    
    return hash;
}

struct sym *store_sym(char *name) {
    struct sym *sp = &symtab[symhash(name) % NAMEHASH];
    int scount = NAMEHASH;
    
    while(--scount >= 0) {
        if(!sp->name) {
            sp->name = strdup(name);
            debug_log("new symbol %s stored at line %d", name, yylineno);
            return sp;
        }
        else if(sp->name && !strcmp(sp->name, name)) {
            return sp;
        }
        
        if(++sp >= symtab + NAMEHASH) {
            sp = symtab;
        }
    }
    
    yyerror("symbol table overflow at line %d", yylineno);
    abort();
    return NULL;
}

struct glosym *register_glosym(char *name) {
    struct glosym *sp = &glosymtab[symhash(name) % GLOSYMTABSIZE];
    int scount = GLOSYMTABSIZE;
    
    while(--scount >= 0) {
        if(!sp->name) {
            sp->name = name; // don't need to duplicate
            debug_log("symbol %s used by global symbol table at line %d", name, yylineno);
            return sp;
        }
        else if(sp->name && !strcmp(sp->name, name)) {
            yyerror("internal error: symbol duplicated register at line %d", yylineno);
            return sp;
        }
        
        if(++sp >= glosymtab + GLOSYMTABSIZE) {
            sp = glosymtab;
        }
    }
    
    yyerror("glosymbol table overflow at line %d", yylineno);
    abort();
    return NULL;
}

struct glosym *lookup_glosym(char *name) {
    struct glosym *sp = &glosymtab[symhash(name) % GLOSYMTABSIZE];
    int scount = GLOSYMTABSIZE - 1;
    
    if(!sp->name) {
        return NULL;
    }
    else if(sp->name && !strcmp(sp->name, name)) {
        return sp;
    }
    
    if(++sp >= glosymtab + GLOSYMTABSIZE) {
        sp = glosymtab;
    }
    
    while(--scount >= 0) {
        if(!sp->name) {
            return NULL;
        }
        else if(sp->name && !strcmp(sp->name, name)) {
            return sp;
        }
        
        if(++sp >= glosymtab + GLOSYMTABSIZE) {
            sp = glosymtab;
        }
    }
    
    return NULL;
}

struct locsym *register_locsym(struct locsym tab[], char *name) {
    struct locsym *sp = &tab[symhash(name) % LOCSYMTABSIZE];
    int scount = LOCSYMTABSIZE;
    
    while(--scount >= 0) {
        if(!sp->name) {
            sp->name = name; // don't need to duplicate
            debug_log("symbol %s used by local symbol table at line %d", name, yylineno);
            return sp;
        }
        else if(sp->name && !strcmp(sp->name, name)) {
            yyerror("internal error: symbol duplicated register at line %d", yylineno);
            return sp;
        }
        
        if(++sp >= tab + LOCSYMTABSIZE) {
            sp = tab;
        }
    }
    
    yyerror("locsymbol table overflow at line %d", yylineno);
    abort();
    return NULL;
}

struct locsym *lookup_locsym(struct locsym tab[], char *name) {
    struct locsym *sp = &tab[symhash(name) % LOCSYMTABSIZE];
    int scount = LOCSYMTABSIZE - 1;
    
    if(!sp->name) {
        return NULL;
    }
    else if(sp->name && !strcmp(sp->name, name)) {
        return sp;
    }
    
    if(++sp >= tab + LOCSYMTABSIZE) {
        sp = tab;
    }
    
    while(--scount >= 0) {
        if(!sp->name) {
            return NULL;
        }
        else if(sp->name && !strcmp(sp->name, name)) {
            return sp;
        }
        
        if(++sp >= tab + LOCSYMTABSIZE) {
            sp = tab;
        }
    }
    
    return NULL;
}

struct ast *make_ast(int nodetype, struct ast *l, struct ast *r) {
    struct ast *a = makesure_malloc(sizeof(struct ast));
    
    a->nodetype = nodetype;
    a->l = l;
    a->r = r;
    
    //debug_log("AST node %d at line %d", nodetype, yylineno);
    
    return a;
}

struct ast *make_funccall(struct sym *s, struct ast *args) {
    struct glosym *gs = lookup_glosym(s->name);
    
    if(!gs || (gs->nodetype != NODETYPE_FUNCDECLR && gs->nodetype != NODETYPE_FUNCIMPL)) {
        yyerror("call to undefined function %s at line %d", s->name, yylineno);
        free_ast(args);
        return NULL;
    }
    
    struct funccall *fc = makesure_malloc(sizeof(struct funccall));
    
    fc->nodetype = NODETYPE_FUNCCALL;
    fc->f = gs;
    
    int nargs;
    struct ast *temp = args;
    if((nargs = ((struct func *)(gs->prop))->nargs) > 0) {
        do { --nargs; temp = temp->r; } while(temp && temp->nodetype == NODETYPE_LIST);
        if(nargs > 0) {
            yyerror("need more %d args for calling to %s at line %d", nargs, gs->name, yylineno);
        } else if(nargs < 0) {
            yyerror("need less %d args for calling to %s at line %d", -nargs, gs->name, yylineno);
        }
        fc->args = args;
    } else {
        fc->args = NULL;
        if(temp && temp->nodetype == NODETYPE_LIST) {
            yyerror("pass args to function %s with no params at line %d", gs->name, yylineno);
        }
        free_ast(args);
    }
    
    debug_log("call to function %s at line %d", gs->name, yylineno);
    
    return (struct ast *)fc;
}

struct ast *make_symref(struct locsym tab[], struct sym *s) {
    struct locsym *ls;
    struct glosym *gs;
    if(tab && (ls = lookup_locsym(tab, s->name))) {
        struct symref *sr = makesure_malloc(sizeof(struct symref));
        sr->nodetype = NODETYPE_LOCREF;
        sr->s = (struct sym *)ls;
        
        debug_log("refer to local var %s at line %d", s->name, yylineno);
        
        return (struct ast *)sr;
        
    } else if(gs = lookup_glosym(s->name)) {
        struct symref *sr = makesure_malloc(sizeof(struct symref));
        sr->nodetype = NODETYPE_GLOREF;
        sr->s = (struct sym *)gs;
        
        debug_log("refer to global var %s at line %d", s->name, yylineno);
        
        return (struct ast *)sr;
        
    } else {
        yyerror("refer to undefined var at line %d", yylineno);
        return NULL;
    }
}

struct ast *make_symasgn(struct locsym tab[], struct sym *s, struct ast *val) {
    struct symref *sr;
    if(sr = (struct symref *)make_symref(tab, s)) {
        struct symasgn *sa = makesure_malloc(sizeof(struct symasgn));
        sa->nodetype = NODETYPE_SYMASGN;
        sa->sr = sr;
        sa->val = val;
        
        debug_log(((struct symref *)sr)->nodetype == NODETYPE_GLOREF ?
            "assign to global var %s at line %d" :
            "assign to local var %s at line %d", s->name, yylineno);
        
        return (struct ast *)sa;
        
    } else {
        free_ast(val);
        return NULL;
    }
}

struct ast *make_flow(int nodetype, struct ast *cond, struct ast *tt, struct ast *ft) {
    struct flow *fl = makesure_malloc(sizeof(struct flow));
    
    fl->nodetype = nodetype;
    fl->cond = cond;
    fl->tt = tt;
    fl->ft = ft;
    
    debug_log(nodetype == 'I' ?
        (ft != NULL ? "if-else block at %d" : "if block at %d") :
        "while block at %d", yylineno);
    
    return (struct ast *)fl;
}

struct ast *make_intval(int val) {
    struct intval *i = makesure_malloc(sizeof(struct intval));
    
    i->datatype = NODETYPE_INT;
    i->val = val;
    
    debug_log("int with val %d at line %d", val, yylineno);
    
    return (struct ast *)i;
}

struct ast *make_charval(char val) {
    struct charval *i = makesure_malloc(sizeof(struct charval));
    
    i->datatype = NODETYPE_CHAR;
    i->val = val;
    
    debug_log("char with val %c(%d) at line %d", val, val, yylineno);
    
    return (struct ast *)i;
}

struct ast *make_floatval(float val) {
    struct floatval *i = makesure_malloc(sizeof(struct floatval));
    
    i->datatype = NODETYPE_FLOAT;
    i->val = val;
    
    debug_log("float with val %f at line %d", val, yylineno);
    
    return (struct ast *)i;
}

struct ast *make_stringval(char *val) {
    struct stringval *i = makesure_malloc(sizeof(struct stringval));
    
    i->datatype = NODETYPE_STRING;
    i->val = val;
    
    debug_log("string with val %s at line %d", val, yylineno);
    
    return (struct ast *)i;
}

struct symlist *make_symlist(struct sym *s, struct symlist *next) {
    struct symlist *sl = makesure_malloc(sizeof(struct symlist));
    
    sl->s = s;
    sl->next = next;
    
    return sl;
}

struct typelist *make_typelist(int datatype, struct typelist *next) {
    struct typelist *tl = makesure_malloc(sizeof(struct typelist));
    
    tl->datatype = datatype;
    tl->next = next;
    
    return tl;
}

struct arglist *make_arglist(struct typelist *tl, struct symlist *sl) {
    struct arglist *args = makesure_malloc(sizeof(struct arglist));
    
    args->tl = tl;
    args->sl = sl;
    
    return args;
}

struct ast *make_glovardef(int datatype, struct symlist *vlist) {
    while(vlist) {
        if(!lookup_glosym(vlist->s->name)) {
            struct glosym *gs = register_glosym(vlist->s->name);
            gs->nodetype = NODETYPE_VAR;
            gs->prop = makesure_malloc(sizeof(struct glovar));
            switch(datatype) {
                case NODETYPE_INT:
                    ((struct glovar *)(gs->prop))->val = make_intval(0);
                    break;
                case NODETYPE_CHAR:
                    ((struct glovar *)(gs->prop))->val = make_charval(0);
                    break;
                case NODETYPE_FLOAT:
                    ((struct glovar *)(gs->prop))->val = make_floatval(0.0);
                    break;
                case NODETYPE_STRING:
                    yyerror("not support string type yet, at line %d", yylineno);
                    free_symlist(vlist);
                    return NULL;
                default:
                    yyerror("unknown data type at line %d", yylineno);
                    free_symlist(vlist);
                    return NULL;
            }
            
        } else {
            yyerror("symbol %s definition duplicate at line %d", vlist->s->name, yylineno);
        }
        vlist = vlist->next;
    }
    
    struct vardef *vd = makesure_malloc(sizeof(struct vardef));
    vd->nodetype = NODETYPE_GLOVARDEF;
    vd->datatype = datatype;
    vd->vlist = vlist;
    
    debug_log("global var definition with type %s at line %d",
        datatype == NODETYPE_INT ? "int" : datatype == NODETYPE_CHAR ? "char" :
        datatype == NODETYPE_FLOAT ? "float" : datatype == NODETYPE_STRING ? "string" : "unknown",
        yylineno);
    
    return (struct ast *)vd;
}

struct ast *make_locvardef(struct locsym tab[], int datatype, struct symlist *vlist) {
    while(vlist) {
        if(!lookup_locsym(tab, vlist->s->name)) {
            struct locsym *ls = register_locsym(tab, vlist->s->name);
            switch(datatype) {
                case NODETYPE_INT:
                    ls->val = make_intval(0);
                    break;
                case NODETYPE_CHAR:
                    ls->val = make_charval(0);
                    break;
                case NODETYPE_FLOAT:
                    ls->val = make_floatval(0.0);
                    break;
                case NODETYPE_STRING:
                    yyerror("not support string type yet, at line %d", yylineno);
                    free_symlist(vlist);
                    return NULL;
                default:
                    yyerror("unknown data type at line %d", yylineno);
                    free_symlist(vlist);
                    return NULL;
            }
            
        } else {
            yyerror("symbol %s definition duplicate at line %d", vlist->s->name, yylineno);
        }
        vlist = vlist->next;
    }
    
    struct vardef *vd = makesure_malloc(sizeof(struct vardef));
    vd->nodetype = NODETYPE_LOCVARDEF;
    vd->datatype = datatype;
    vd->vlist = vlist;
    
    debug_log("local var definition with type %s at line %d", DATATYPE_NAME(datatype), yylineno);
    
    return (struct ast *)vd;
}

struct funcdef *make_funcinfo(int retntype, struct sym *f, struct arglist *args) {
    struct funcdef *finfo = makesure_malloc(sizeof(struct funcdef));
    finfo->retntype = retntype;
    finfo->f = f;
    finfo->args = args;
    return finfo;
}

struct ast *make_funcdeclr(struct funcdef *finfo, char impl) {
    if(!lookup_glosym(finfo->f->name)) {
        struct glosym *gs = register_glosym(finfo->f->name);
        gs->nodetype = NODETYPE_FUNCDECLR;
        gs->prop = makesure_malloc(sizeof(struct func));
        
        int nargs = 0;
        struct typelist *tl = finfo->args->tl;
        while(tl) {
            ++nargs;
            tl = tl->next;
        }
        
        struct func *fprop = (struct func *)(gs->prop);
        
        fprop->retntype = finfo->retntype;
        fprop->nargs = nargs;
        fprop->args = finfo->args;
        
        if(!impl) {
            #ifdef DEBUG
            
            tl = finfo->args->tl;
            struct symlist *sl = finfo->args->sl;
            printf("function %s %s (", DATATYPE_NAME(finfo->retntype), finfo->f->name);
            if(nargs == 0) {
                printf("void");
            } else {
                printf("%s %s", DATATYPE_NAME(tl->datatype), sl->s->name);
                for(int i = 1; i < nargs; ++i) {
                    tl = tl->next;
                    sl = sl->next;
                    printf(", %s %s", DATATYPE_NAME(tl->datatype), sl->s->name);
                }
            }
            printf(") with %d args declared at line %d\n", nargs, yylineno);
            
            #endif
            
            finfo->nodetype = NODETYPE_FUNCDECLRAST;
            
            return (struct ast *)finfo;
        } else {
            return NULL;
        }
        
    } else {
        yyerror("symbol %s definition duplicate at line %d", finfo->f->name, yylineno);
        free_arglist(finfo->args);
        free(finfo);
        return NULL;
    }
}

static void bind_formalparams(struct func *fprop) {
    int nargs = fprop->nargs;
    struct typelist *tl = fprop->args->tl;
    struct symlist *sl = fprop->args->sl;
    while(--nargs >= 0) {
        if(!lookup_locsym(fprop->loctab, sl->s->name)) {
            struct locsym *ls = register_locsym(fprop->loctab, sl->s->name);
            switch(tl->datatype) {
                case NODETYPE_INT:
                    ls->val = make_intval(0);
                    break;
                case NODETYPE_CHAR:
                    ls->val = make_charval(0);
                    break;
                case NODETYPE_FLOAT:
                    ls->val = make_floatval(0.0);
                    break;
                case NODETYPE_STRING:
                    yyerror("not support string type yet, at line %d", yylineno);
                    break;
                default:
                    yyerror("unknown data type at line %d", yylineno);
                    break;
            }
            
        } else {
            yyerror("symbol %s definition duplicate at line %d", sl->s->name, yylineno);
        }
        tl = tl->next;
        sl = sl->next;
    }
    
    debug_log("binding formal parameters at line %d", yylineno);
}

struct funcdef *make_funcimplheader(struct funcdef *finfo) {
    struct glosym *gs = lookup_glosym(finfo->f->name);
    struct func *fprop;
    if(!gs) {
        make_funcdeclr(finfo, 1);
        gs = lookup_glosym(finfo->f->name);
        fprop = (struct func *)(gs->prop);
        
        // drop out
        
    } else if(gs->nodetype == NODETYPE_FUNCDECLR) {
        fprop = (struct func *)(gs->prop);
        
        // check if props are same
        int nargs = 0;
        struct typelist *tl = finfo->args->tl;
        struct typelist *otl = fprop->args->tl;
        while(tl) {
            ++nargs;
            if(!otl || tl->datatype != otl->datatype) {
                yyerror("function %s implementation incompatible at line %d", finfo->f->name, yylineno);
                free_arglist(finfo->args);
                free(finfo);
                return finfo;
            }
            tl = tl->next;
            otl = otl->next;
        }
        if(finfo->retntype != fprop->retntype || nargs != fprop->nargs) {
            yyerror("function %s implementation incompatible at line %d", finfo->f->name, yylineno);
            free_arglist(finfo->args);
            free(finfo);
            return finfo;
        }
        
        // The funcdeclr-making funcdef-astnode still holds the old arglist's handle so don't free it
        fprop->args = finfo->args;
        
        // drop out
        
    } else {
        yyerror(gs->nodetype == NODETYPE_FUNCIMPL ?
            "function %s implementation duplicate at line %d" :
            "symbol %s definition duplicate at line %d", finfo->f->name, yylineno);
        free_arglist(finfo->args);
        free(finfo);
        return finfo;
    }
    
    fprop->loctab = makesure_malloc(sizeof(struct locsym) * LOCSYMTABSIZE);
    
    // IMPORTANT for yy-parsing
    yyloctab = fprop->loctab;
    
    bind_formalparams(fprop);
    
    #ifdef DEBUG
    
    struct typelist *tl = finfo->args->tl;
    struct symlist *sl = finfo->args->sl;
    printf("function %s %s (", DATATYPE_NAME(finfo->retntype), finfo->f->name);
    if(tl == NULL || sl == NULL) {
        printf("void");
    } else {
        printf("%s %s", DATATYPE_NAME(tl->datatype), sl->s->name);
        while(tl->next != NULL && sl->next != NULL) {
            tl = tl->next;
            sl = sl->next;
            printf(", %s %s", DATATYPE_NAME(tl->datatype), sl->s->name);
        }
    }
    printf(") with %d args implementation begin at line %d\n", fprop->nargs, yylineno);
    #endif
    
    finfo->nodetype = NODETYPE_FUNCIMPLAST;
    
    return finfo;
}

struct ast *make_funcimpl(struct funcdef *finfo, struct ast *body) {
    struct glosym *gs = lookup_glosym(finfo->f->name);
    struct func *fprop = (struct func *)(gs->prop);
    
    fprop->body = body;
    
    // back to global symbol table
    yyloctab = NULL;
    
    #ifdef DEBUG
    
    int nargs = fprop->nargs;
    struct typelist *tl = fprop->args->tl;
    struct symlist *sl = fprop->args->sl;
    printf("function %s %s (", DATATYPE_NAME(fprop->retntype), finfo->f->name);
    if(nargs == 0) {
        printf("void");
    } else {
        printf("%s %s", DATATYPE_NAME(tl->datatype), sl->s->name);
    }
    for(int i = 1; i < nargs; ++i) {
        tl = tl->next;
        sl = sl->next;
        printf(", %s %s", DATATYPE_NAME(tl->datatype), sl->s->name);
    }
    printf(") with %d args implementation end at line %d\n", nargs, yylineno);
    
    #endif
    
    return (struct ast *)finfo;
}

void *eval_ast(struct ast *a) {
    
}

void free_ast(struct ast *a) {
    if(!a) {
        return;
    }
    switch(a->nodetype) {
        case NODETYPE_STRING:
            free(((struct stringval *)a)->val);
            break;
            
        case NODETYPE_IF:
            free_ast(((struct flow *)a)->ft);
            
        case NODETYPE_WHILE
            free_ast(((struct flow *)a)->cond);
            free_ast(((struct flow *)a)->tt);
            break;
            
        case NODETYPE_SYMASGN:
            free(((struct symasgn *)a)->sr);
            free_ast(((struct symasgn *)a)->val);
            
        case NODETYPE_GLOREF:
        case NODETYPE_LOCREF:
            break;
            
        case NODETYPE_FUNCCALL:
            free_ast(((struct funccall *)a)->args);
            break;
            
        case NODETYPE_GLOVARDEF:
        case NODETYPE_LOCVARDEF:
            free_symlist(((struct vardef *)a)->vlist);
            break;
            
        case NODETYPE_FUNCDECLRAST:
        case NODETYPE_FUNCIMPLAST:
            free_arglist(((struct funcdef *)a)->args);
            break;
            
        case NODETYPE_LIST:
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case NODETYPE_SHL:
        case NODETYPE_SHR:
        case '<':
        case '>':
        case NODETYPE_LE:
        case NODETYPE_GE:
        case NODETYPE_EQ:
        case NODETYPE_NE:
        case '&':
        case '^':
        case '|':
        case NODETYPE_LAND:
        case NODETYPE_LOR:
            free(a->r);
            
        case NODETYPE_NEGATIVE:
        case NODETYPE_POSITIVE:
        case '~':
        case '!':
        case NODETYPE_PREINC:
        case NODETYPE_PREDEC:
        case NODETYPE_POSTINC:
        case NODETYPE_POSTDEC:
        case NODETYPE_SIZEOF:
        case NODETYPE_RETURN:
            free(a->l);
            
        case NODETYPE_INT:
        case NODETYPE_CHAR:
        case NODETYPE_FLOAT:
        case NODETYPE_CONTINUE:
        case NODETYPE_BREAK:
            break;
            
        default:
            yyerror("internal error: unknown node-type at line %d", yylineno);
    }
    free(a);
}

void free_symlist(struct symlist *sl) {
    struct symlist *temp1 = sl;
    struct symlist *temp2 = temp1;
    while(temp2) {
        temp2 = temp2->next;
        free(temp1);
        temp1 = temp2;
    }
}

void free_typelist(struct typelist *tl) {
    struct typelist *temp1 = tl;
    struct typelist *temp2 = temp1;
    while(temp2) {
        temp2 = temp2->next;
        free(temp1);
        temp1 = temp2;
    }
}

void free_arglist(struct arglist *al) {
    free_typelist(al->tl);
    free_symlist(al->sl);
    free(al);
}

void yyerror(char *s, ...) {
    va_list ap;
    va_start(ap, s);
    
    fprintf(stderr, "%d: error: ", yylineno);
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
}
