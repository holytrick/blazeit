#include <stdio.h>
#include <stdlib.h>
#include "term.h"
#include "environment.h"
#include "utils.h"
#include "parser.h"

#define debug 1

void blazeit(FILE *input, environment env)
{
    while(1){
        if(debug){
            printf("~ ");
            fflush(stdout);
        }
        char *line = fgetl(input);
        if(!line){
            printf("EOF\n");
            break;
        }
        term *t = parse_string(line);
        if(debug){
            printf("Input: ");
            print_term(t);
            printf("\n");
        }
        term *type = type_infer(t, env, 0);
        if(debug){
            printf("Type Check: ");
            print_term(type);
            printf("\n");
        }
        if(type) evaluate_term(t, env);
        else fprintf(stderr, "Didn't Type Check!\n");

        if(debug){
            printf("Output: ");
            print_term(t);
            printf("\n");
        }
        free_term(type);
        free_term(t);
        free(line);
    }
}

int main(int argc, char **argv)
{
    int i;
    environment env = make_environment();
    for(i = 1; i < argc; ++i){
        FILE *fp = fopen(argv[i], "r");
        if(!fp) file_error(argv[i]);
        blazeit(fp, env);
        fclose(fp);
    }
    blazeit(stdin, env);
    free_environment(env);

    return 0;
}

