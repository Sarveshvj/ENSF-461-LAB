#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "parser.h"

#define BUFLEN 1024
#define MAXARGS 64 

extern char **environ;

//To Do: This base file has been provided to help you start the lab, you'll need to heavily modify it to implement all of the features

int tokenize(const char *line, char **argv, int maxargs);
void free_tokens(char** argv);

int main(void) {
    char buffer[BUFLEN];
    char* parsedinput;
    char* argv[MAXARGS];
    char* newline;

    printf("Welcome to the Group23 shell! Enter commands, enter 'quit' to exit\n");
    do {
        //Print the terminal prompt and get input
        printf("$ ");
        fflush(stdout);
        char *input = fgets(buffer, sizeof(buffer), stdin);
        if(!input)
        {
            fprintf(stderr, "Error reading input\n");
            return -1;
        }
        // Trim newline
        newline = strchr(input, '\n');
        if(newline) *newline = '\0';
        
        //Clean and parse the input string
        parsedinput = (char*) malloc(BUFLEN * sizeof(char));
        if(!parsedinput) {perror("malloc"); return 1;}
        size_t parselength = trimstring(parsedinput, input, BUFLEN);
        if(parselength == 0) {free(parsedinput); continue;}
        
        //Sample shell logic implementation
        if ( strcmp(parsedinput, "quit") == 0 ) {
            printf("Bye!!\n");
            free(parsedinput);
            return 0;
        }
        else {
            int argc = tokenize(parsedinput, argv, MAXARGS);
            if (argc == -1) {          
                fprintf(stderr, "Error: unterminated quote\n");
            } 
            else if (argc == -2) {       
                fprintf(stderr, "Error: too many arguments\n");
            } 
            else if (argc < 0) {                     
                fprintf(stderr, "Error: out of memory\n");
            } 
            else if (argc == 0) {               
                fprintf(stderr, "Error: missing command\n");
            }
            else{
                pid_t forkV = fork();
                if(forkV < 0){
                    perror("fork");            
                }
                else if (forkV > 0){
                    //Parent process
                    if(waitpid(forkV, NULL, 0) < 0) perror("waitpid");
                }
                else{

                    execve(argv[0], argv, environ);
                    perror("execve");
                    _exit(1);
                }
                free_tokens(argv);
            
            }
        }

        //Remember to free any memory you allocate!
        free(parsedinput);
    } while ( 1 );

    return 0;
}



int tokenize(const char* line, char** argv, int maxargs)
{
    int argc = 0;
    const char* p = line;
    argv[0] = NULL;
 
    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
 
        if (argc >= maxargs - 1) {
            free_tokens(argv);
            return -2;
        }
 
        char* tok = malloc(strlen(p) + 1);
        if (!tok) {
            free_tokens(argv);
            return -3;
        }
 
        size_t k = 0;
        char quote = 0;
        while (*p) {
            if (quote) {
                if (*p == quote) quote = 0;
                else tok[k++] = *p;
            } else if (*p == '"' || *p == '\'') {
                quote = *p;
            } else if (*p == ' ' || *p == '\t') {
                break;
            } else {
                tok[k++] = *p;
            }
            p++;
        }
 
        if (quote) {
            free(tok);
            free_tokens(argv);
            return -1;
        }
 
        tok[k] = '\0';
        argv[argc++] = tok;
        argv[argc] = NULL;
    }
    return argc;
}

void free_tokens(char** argv){
    for(int i =0; argv[i] != NULL; i++){
        free(argv[i]);
        argv[i] = NULL;
    }
}