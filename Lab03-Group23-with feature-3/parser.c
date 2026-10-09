#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include <unistd.h>
#include <sys/stat.h>

size_t trimstring(char* outputbuffer, const char* inputbuffer, size_t bufferlen)
{
    if (bufferlen == 0) return 0;

    size_t inlen = strnlen(inputbuffer, bufferlen - 1);
    size_t start = 0;
    while (start < inlen && (unsigned char)inputbuffer[start] < '!')
        start++;

    size_t len = inlen - start;
    memmove(outputbuffer, inputbuffer + start, len);
    outputbuffer[len] = '\0';

    while (len > 0 && (unsigned char)outputbuffer[len - 1] < '!')
        outputbuffer[--len] = '\0';

    return len;
}

size_t firstword(char* outputbuffer, const char* inputbuffer, size_t bufferlen)
{
    if (bufferlen == 0) return 0;
    size_t i = 0;
    while (i < bufferlen - 1 &&
           inputbuffer[i] != '\0' &&
           (unsigned char)inputbuffer[i] >= '!')
    {
        outputbuffer[i] = inputbuffer[i];
        i++;
    }
    outputbuffer[i] = '\0';
    return i;
}

bool isvalidascii(const char* inputbuffer, size_t bufferlen)
{
    size_t testlen = strnlen(inputbuffer, bufferlen);
    for (size_t ii = 0; ii < testlen; ii++) {
        unsigned char c = (unsigned char)inputbuffer[ii];
        if (c < ' ' || c > '~') return false;
    }
    return true;
}

int findpipe(const char* inputbuffer, size_t bufferlen)
{
    size_t n = strnlen(inputbuffer, bufferlen);
    char quote = 0;
    for (size_t i = 0; i < n; i++) {
        char c = inputbuffer[i];
        if (quote) {
            if (c == quote) quote = 0;
        } else if (c == '"' || c == '\'') {
            quote = c;
        } else if (c == '|') {
            return (int)i;
        }
    }
    return -1;
}

void free_tokens(char** argv)
{
    for (int i = 0; argv[i] != NULL; i++) {
        free(argv[i]);
        argv[i] = NULL;
    }
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

static bool is_executable_file(const char* path)
{
    struct stat st;
    if (stat(path, &st) != 0) return false;
    if (!S_ISREG(st.st_mode)) return false;
    return access(path, X_OK) == 0;
}

int resolve_path(const char* cmd, char* outpath, size_t outlen)
{
    if (cmd[0] == '\0') return -1;

    if (strchr(cmd, '/') != NULL) {
        if (strlen(cmd) >= outlen) return -1;
        if (!is_executable_file(cmd)) return -1;
        strcpy(outpath, cmd);
        return 0;
    }

    const char* pathenv = getenv("PATH");
    if (pathenv == NULL) pathenv = "/usr/bin:/bin";

    char* copy = strdup(pathenv);
    if (!copy) { perror("strdup"); return -1; }

    int result = -1;
    char* saveptr = NULL;
    for (char* dir = strtok_r(copy, ":", &saveptr);
         dir != NULL;
         dir = strtok_r(NULL, ":", &saveptr))
    {
        int n = snprintf(outpath, outlen, "%s/%s", dir, cmd);
        if (n < 0 || (size_t)n >= outlen) continue;
        if (is_executable_file(outpath)) {
            result = 0;
            break;
        }
    }
    free(copy);
    return result;
}
