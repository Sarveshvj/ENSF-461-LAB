#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "parser.h"

#define BUFLEN 1024

extern char **environ;

static void wait_for(pid_t pid)
{
    while (waitpid(pid, NULL, 0) < 0) {
        if (errno == EINTR) continue;
        perror("waitpid");
        return;
    }
}




static void close_fd(int fd)
{
    if (close(fd) < 0) {
        perror("close");
    }
}




static int prepare_command(const char* text, char** argv, char* path, size_t pathlen)
{
    int argc = tokenize(text, argv, MAXARGS);
    if (argc == -1) { fprintf(stderr, "Error: unterminated quote\n"); return -1; }
    if (argc == -2) { fprintf(stderr, "Error: too many arguments\n"); return -1; }
    if (argc < 0)   { fprintf(stderr, "Error: out of memory\n"); return -1; }
    if (argc == 0)  { fprintf(stderr, "Error: missing command\n"); return -1; }

    if (resolve_path(argv[0], path, pathlen) != 0) {
        fprintf(stderr, "%s: command not found\n", argv[0]);
        free_tokens(argv);
        return -1;
    }
    return 0;
}

static void run_single(const char* line)
{
    char* argv[MAXARGS];
    char path[BUFLEN];
    if (prepare_command(line, argv, path, sizeof(path)) != 0) return;

    fflush(stdout);
    pid_t forkV = fork();
    if (forkV < 0) {
        perror("fork");
    } else if (forkV == 0) {
        execve(path, argv, environ);
        perror("execve");
        _exit(127);
    } else {
        wait_for(forkV);
    }
    free_tokens(argv);
}

static void run_pipeline(const char* line, int pipeidx)
{
    char left[BUFLEN];
    memcpy(left, line, (size_t)pipeidx);
    left[pipeidx] = '\0';
    const char* right = line + pipeidx + 1;

    if (findpipe(right, strlen(right) + 1) >= 0) {
        fprintf(stderr, "Error: only a single pipe is supported\n");
        return;
    }

    char* argv1[MAXARGS];
    char* argv2[MAXARGS];
    char path1[BUFLEN], path2[BUFLEN];

    if (prepare_command(left, argv1, path1, sizeof(path1)) != 0) return;
    if (prepare_command(right, argv2, path2, sizeof(path2)) != 0) {
        free_tokens(argv1);
        return;
    }

    int fd[2];
    if (pipe(fd) < 0) {
        perror("pipe");
        goto done;
    }

    fflush(stdout);
    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        close_fd(fd[0]);
        close_fd(fd[1]);
        goto done;
    }
    if (pid1 == 0) {
        if (dup2(fd[1], STDOUT_FILENO) < 0) {
            perror("dup2"); _exit(1);
        }
        close_fd(fd[0]);
        close_fd(fd[1]);
        execve(path1, argv1, environ);
        perror("execve");
        _exit(127);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        close_fd(fd[0]);
        close_fd(fd[1]);
        wait_for(pid1);
        goto done;
    }
    if (pid2 == 0) {
        if (dup2(fd[0], STDIN_FILENO) < 0) {
            perror("dup2"); _exit(1);
        }
        close_fd(fd[0]);
        close_fd(fd[1]);
        execve(path2, argv2, environ);
        perror("execve");
        _exit(127);
    }
    close_fd(fd[0]);
    close_fd(fd[1]);

    wait_for(pid1);
    wait_for(pid2);

done:
    free_tokens(argv1);
    free_tokens(argv2);
}

int main() {
    char buffer[BUFLEN];
    char* parsedinput;
    char* newline;

    printf("Welcome to the Group23 shell! Enter commands, enter 'quit' to exit\n");
    do {

        printf("$ ");
        fflush(stdout);
        char *input = fgets(buffer, sizeof(buffer), stdin);
        if(!input)
        {
            if (feof(stdin)) { printf("\n"); return 0; }
            perror("fgets");
            return -1;
        }

        if (strchr(input, '\n') == NULL && !feof(stdin)) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            fprintf(stderr, "Error: input too long\n");
            continue;
        }

        newline = strchr(input, '\n');
        if(newline) *newline = '\0';

        parsedinput = (char*) malloc(BUFLEN * sizeof(char));
        if(!parsedinput) {perror("malloc"); return 1;}
        size_t parselength = trimstring(parsedinput, input, BUFLEN);

        if(parselength == 0) {free(parsedinput); continue;}

        if(!isvalidascii(parsedinput, BUFLEN)) {
            fprintf(stderr, "Error: input contains invalid characters\n");
            free(parsedinput);
            continue;
        }

        if ( strcmp(parsedinput, "quit") == 0 ) {
            printf("Bye!!\n");
            free(parsedinput);
            return 0;
        }
        else {
            int pipeidx = findpipe(parsedinput, BUFLEN);
            if (pipeidx >= 0) run_pipeline(parsedinput, pipeidx);
            else              run_single(parsedinput);
        }

        free(parsedinput);
    } while ( 1 );

    return 0;
}
