#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "term.h"
#include "utils.h"
#include "environment.h"

/*
    if (t->kind == ANN){
    }else if (t->kind == VAR){
    }else if (t->kind == APP){
    }else if (t->kind == FUN){
    }else if (t->kind == IND){
    }else if (t->kind == PI){
    }else if (t->kind == TYPE){
    }
*/

term *copy_term(term *t)
{
    if(!t) return 0;
    term *copy = calloc(1, sizeof(term));
    copy->kind = t->kind;
    copy->left  = copy_term(t->left);
    copy->right = copy_term(t->right);
    copy->annotation = copy_term(t->annotation);
    if (t->name) copy->name = copy_string(t->name);
    copy->n = t->n;
    if (t->kind == IND){
        not_implemented();
    }
    return copy;
}

void increment(term *t, int n, int level)
{
    if (!t) return;
    if (t->annotation) increment(t->annotation, n, level);
    if (t->kind == VAR){
        if (t->n >= level) t->n += n;
    }else if (t->kind == APP){
        increment(t->left,  n, level);
        increment(t->right, n, level);
    }else if (t->kind == FUN || t->kind == PI){
        increment(t->left->annotation, n, level);
        increment(t->right, n, level+1);
    }else if (t->kind == IND){
    }else if (t->kind == TYPE){
    }
}

void substitute(term *t, term *arg, int level)
{
    if(!t) return;
    if (t->kind == VAR){
        if(t->n == level){
            term *copy = copy_term(arg);       
            increment(copy, level, 0);
            if (t->name) free(t->name);
            *t = *copy;
            free(copy);
        }else if(t->n > level){
            t->n -= 1;
        }
    }else if (t->kind == APP){
        substitute(t->left, arg, level);
        substitute(t->right, arg, level);
    }else if (t->kind == IND){
    }else if (t->kind == PI || t->kind == FUN){
        substitute(t->left->annotation, arg, level);
        substitute(t->right, arg, level+1);
    }else if (t->kind == TYPE){
    }
}

void free_term(term *t)
{
    if(!t) return;
    free_term(t->left);
    free_term(t->right);
    free_term(t->annotation);
    if (t->name) free(t->name);
    free(t);
}

void replace_term(term *old, term *new)
{
    term *copy = copy_term(new);
    free_term(old->left);
    free_term(old->right);
    if (old->name) free(old->name);
    *old = *copy;
    free(copy);
}

environment evaluate_term(term *t, environment env)
{
    if(!t) return env;
    if (t->annotation) evaluate_term(t->annotation, env);
    if (t->kind == DEF){
        evaluate_term(t->right, env);
        free_term(t->right->annotation);
        t->right->annotation = copy_term(t->left->annotation);
        set_environment(env, t);
    }
    if (t->kind == VAR){
        term *lookup = 0;
        if (t->name) lookup = get_environment(env, t->name);
        if (lookup){
            replace_term(t, lookup);
            evaluate_term(t, env);
        }
    }
    if (t->kind == FUN){
        evaluate_term(t->left, env);
        evaluate_term(t->right, env);
    }
    if (t->kind == PI){
        evaluate_term(t->left, env);
        evaluate_term(t->right, env);
    }
    if (t->kind == APP){
        evaluate_term(t->left, env);
        evaluate_term(t->right, env);
        if (t->left->kind == FUN){
            
            term *lam = t->left;
            term *arg = copy_term(t->right);

            replace_term(t, lam->right);

            substitute(t, arg, 0);

            free_term(arg);
            evaluate_term(t, env);
        }
    }
    return env;
}

int compare_types(term *t1, term *t2)
{
    //printf("Comparing ");
    //print_term(t1);
    //printf(" and ");
    //print_term(t2);
    //printf("\n");

    if(!t1 && !t2) return 1;
    if(!t1 || !t2) return 0;
    if(t1->kind != t2->kind) return 0;
    if(t1->kind == VAR) return t1->n == t2->n;
    if(t1->kind == TYPE && t2->kind == TYPE) return 1;
    return (compare_types(t1->left, t2->left) && compare_types(t1->right, t2->right));
}

term *type_infer(term *t, environment env, term_list *context)
{
    if(t->kind == VAR){
        term *l = 0;
        if (t->name) l = get_environment(env, t->name);
        else l = get_term_list(context, t->n);
        if (!l) {
            //printf("Couldn't find var ");
            //print_term(t);
            //printf(" in context or env\n");
            return 0;
        }
        term *m = copy_term(l->annotation);
        //printf("Before increment:");
        //print_term(m);
        //printf("\n");
        increment(m, t->n+1, 0);
        //printf("increment by %d\n", t->n+1);

        //printf("Inferred ");
        //print_term(t);
        //printf(" has type ");
        //print_term(m);
        //printf("\n");

        return m;
    }
    if(t->kind == FUN){
        term *arg = t->left;
        term *body = t->right;
        evaluate_term(arg, env);
        context = push_term_list(context, arg);
        term* infer = type_infer(body, env, context);
        context = pop_term_list(context);
        if(!infer){
            //printf("Couldn't infer Body type ");
            //print_term(body);
            //printf("\n");
            return 0;
        }
        term *pi = calloc(1, sizeof(term));
        pi->kind = PI;
        pi->left = copy_term(arg);
        pi->right = infer;

        //printf("Inferred ");
        //print_term(t);
        //printf(" has type ");
        //print_term(pi);
        //printf("\n");

        return pi;
    }
    if(t->kind == APP){
        term *f = t->left;
        term *x = t->right;
        term *pi = type_infer(f, env, context);
        if(!pi) return 0;
        if (pi->kind != PI){
            //printf("APP doesn't have PI type\n");
            free_term(pi);
            return 0;
        }
        term *T = pi->right;
        term *S = pi->left->annotation;
        if (!S){
            //printf("PI variable doesn't have annotation\n");
            return 0;
        }
        int check = type_check(x, env, context, S);
        if (!check) return 0;
        term *sub = copy_term(T);
        //printf("Subbing ");
        //print_term(x);
        //printf(" in ");
        //print_term(sub);
        //printf("\n");
        substitute(sub, x, 0);

        free_term(pi);

        //printf("Inferred ");
        //print_term(t);
        //printf(" has type ");
        //print_term(sub);
        //printf("\n");
        return sub;
    }
    if(t->kind == PI){
        term *base = calloc(1, sizeof(term));
        base->kind = TYPE;

        //printf("Inferred ");
        //print_term(t);
        //printf(" has type ");
        //print_term(base);
        //printf("\n");

        return base;
    }
    if(t->kind == TYPE){
        term *base = calloc(1, sizeof(term));
        base->kind = TYPE;
        base->n = t->n+1;

        //printf("Inferred ");
        //print_term(t);
        //printf(" has type ");
        //print_term(base);
        //printf("\n");

        return base;
    }
    if(t->kind == DEF){
        if(t->left->annotation){
            int check = type_check(t->right, env, context, t->left->annotation);
            if(check) return copy_term(t->left->annotation);
        }else{
            term *infer = type_infer(t->right, env, context);    
            t->left->annotation = copy_term(infer);
            return infer;
        }
        return 0;
    }
    return 0;
}

int type_check(term *t, environment env, term_list *context, term *type)
{
    evaluate_term(type, env);
    //printf("check ");
    //print_term(t);
    //printf(" is type ");
    //print_term(type);
    //printf("\n");
    //printf("Context: ");
    //print_context(c);
    //VAR, APP, FUN, IND, PI, TYPE, CONS, DEF
    if(type->kind == PI){
        if(t->kind == FUN){
            term *arg_type = type->left->annotation;
            term *arg = t->left;
            evaluate_term(arg, env);
            if(arg->annotation){
                if(!compare_types(arg->annotation, arg_type)){
                    //printf("Arguement doesn't match annotation\n");
                    return 0;
                }
            }else{
                arg->annotation = copy_term(arg_type);
            }
            term *body = t->right;
            term *body_type = type->right;

            context = push_term_list(context, arg);
            int check = type_check(body, env, context, body_type);
            context = pop_term_list(context);
            return check;
        }
        if(t->kind == PI || t->kind == TYPE || t->kind == IND) return 0;
    }else if(type->kind == TYPE){
        if(t->kind == PI || t->kind == TYPE || t->kind == IND) return 1;
        if(t->kind == FUN) return 0;
    } else if(type->kind == VAR){

    } else {
        //printf("Bad: ");
        //print_term(type);
        //printf("\n");
        error("Fuck");
    }

    if (t->kind == CONS || t->kind == DEF) return 0;

    if (t->kind == VAR){
        term *l = 0;
        if (t->name) l = get_environment(env, t->name);
        else l = get_term_list(context, t->n);
        if (!l){
            //printf("VAR not found in env or context\n");
            return 0;
        }
        term *m = copy_term(l->annotation);
        if(!m){
            //print_term(l);
            //printf(" No annotation for VAR\n");
        }
        increment(m, t->n+1, 0);
        int compare = compare_types(m, type);
        free_term(m);
        if (compare) return 1;
        //printf("VAR doesn't match type\n");
        return 0;
    } else if (t->kind == APP){
        term *infer = type_infer(t, env, context);
        int compare = compare_types(infer, type);
        free_term(infer);
        if (compare) return 1;
        //printf("APP doesn't match type\n");
        return 0;
    }
    else{
        error("Shit");
    }
    return 0;
}

typedef struct index_list{
    char *name;
    struct index_list *next;
} index_list;

void debruijn_r(term *t, index_list *list)
{
    //printf("debruge\n");
    //print_term(t);
    //printf("\n");
    if (!t) return;
    if (t->annotation) debruijn_r(t->annotation, list);
    if (t->kind == VAR){
        int count = 0;
        while(list){
            //printf("...%s...", list->name);
            if(strcmp(list->name, t->name) == 0){
                t->n = count;
                free(t->name);
                t->name = 0;
                break;
            }
            ++count;
            list = list->next;
        }
        //printf("\n");
        t->n = count;
    }else if (t->kind == DEF){
        debruijn_r(t->right, list);
    }else if (t->kind == APP){
        debruijn_r(t->left, list);
        debruijn_r(t->right, list);
    }else if (t->kind == FUN || t->kind == PI){
        debruijn_r(t->left->annotation, list);
        index_list *top = calloc(1, sizeof(index_list));
        top->name = t->left->name;
        //printf("Adding %s to list\n", top->name);

        top->next = list;
        debruijn_r(t->right, top);
        //printf("Popping %s from list\n", top->name);
        free(top);
    }else if (t->kind == IND){
    }else if (t->kind == TYPE){
    }
}

void debruijn(term *t)
{
    debruijn_r(t, 0);
}

void print_term_r(term *t, index_list *list);

void print_term_fun(term *t, index_list *list)
{
    printf("(fun ");
    index_list *top = list;
    while(t && t->kind == FUN){
        print_term_r(t->left, top);
        index_list *next = top;
        top = calloc(1, sizeof(index_list));
        top->name = t->left->name;
        top->next = next;
        t = t->right;
        printf(" ");
    }
    /*
    if(t->annotation){
        printf(" : ");
        print_term_r(t->annotation, list);
    }
    */
    printf(" => ");

    print_term_r(t, top);
    while(top != list){
        index_list *next = top->next;
        free(top);
        top = next;
    }

    printf(")");
}

void print_term_r(term *t, index_list *list)
{
    if(!t){
        printf("Null Term");
        return;
    }
    index_list *front = list;
    if (t->kind == VAR){
        if(t->name && strcmp(t->name, "_")==0){
            print_term_r(t->annotation, front);
            return;
        }

        if(t->name){
            printf("%s", t->name);
        }else{
            int i;
            for(i = 0; i < t->n; ++i){
                if(!list) break;
                list = list->next;
            }
            if(!list) printf("%d", t->n);
            else printf("%s", list->name);
        }
        /*
        if(t->annotation){
            printf(":(");
            print_term_r(t->annotation, front);
            printf(")");
        }
        */
    }else if (t->kind == DEF){
        printf("def ");
        print_term_r(t->left, list);
        printf(" = ");
        print_term_r(t->right, list);
    }else if (t->kind == APP){
        printf("(");
        print_term_r(t->left, list);
        printf(" ");
        print_term_r(t->right, list);
        printf(")");
    }else if (t->kind == FUN){
        print_term_fun(t, list);
    }else if (t->kind == IND){
    }else if (t->kind == PI){
        printf("(");
        print_term_r(t->left, list);
        printf(" -> ");
        index_list *top = calloc(1, sizeof(index_list));
        top->name = t->left->name;
        top->next = list;

        print_term_r(t->right, top);

        free(top);
        printf(")");
    }else if (t->kind == TYPE){
        printf("Type");
    }
}

void print_term(term *t)
{
    print_term_r(t, 0);
}

