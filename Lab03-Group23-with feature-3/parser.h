#ifndef __PARSER_H
#define __PARSER_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>

#define MAXARGS 64

size_t trimstring(char* outputbuffer, const char* inputbuffer, size_t bufferlen);
size_t firstword(char* outputbuffer, const char* inputbuffer, size_t bufferlen);
bool isvalidascii(const char* inputbuffer, size_t bufferlen);
int findpipe(const char* inputbuffer, size_t bufferlen);

int tokenize(const char* line, char** argv, int maxargs);

void free_tokens(char** argv);

int resolve_path(const char* cmd, char* outpath, size_t outlen);

#endif
