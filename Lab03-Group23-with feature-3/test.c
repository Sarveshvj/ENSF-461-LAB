#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include "parser.h"

#define BUFLEN 1024

int main() {
    printf("Testing Parser Implementation\n");

    char teststring1[BUFLEN] = "command arg1 arg2";
    char teststring2[BUFLEN] = "This is a test string   \n";
    char teststring3[BUFLEN] = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64, 0xED, 0x00};
    char teststring4[BUFLEN] = "test pipe | parsing\n";
    char out[BUFLEN];

    printf("Testing String Trimming\n");
    trimstring(out, teststring1, BUFLEN);
    assert(strlen(teststring1) == strlen(out));
    trimstring(out, teststring2, BUFLEN);
    assert(strlen(teststring2) > strlen(out));
    assert(strcmp(out, "This is a test string") == 0);
    char blankline[BUFLEN] = "   \n";
    assert(trimstring(out, blankline, BUFLEN) == 0);
    char emptyline[BUFLEN] = "";
    assert(trimstring(out, emptyline, BUFLEN) == 0);
    char leading[BUFLEN] = "   ls -l";
    trimstring(out, leading, BUFLEN);
    assert(strcmp(out, "ls -l") == 0);

    printf("Testing ASCII String Validation\n");
    assert(isvalidascii(teststring1, BUFLEN) == true);
    assert(isvalidascii(teststring2, BUFLEN) == false);
    assert(isvalidascii(teststring3, BUFLEN) == false);

    printf("Testing firstword\n");
    assert(firstword(out, teststring1, BUFLEN) == 7);
    assert(strcmp(out, "command") == 0);
    assert(firstword(out, "single", BUFLEN) == 6);
    assert(firstword(out, "", BUFLEN) == 0);

    printf("Testing findpipe\n");
    assert(findpipe(teststring4, BUFLEN) == 10);
    assert(findpipe(teststring1, BUFLEN) == -1);
    assert(findpipe("echo \"a|b\"", BUFLEN) == -1);
    assert(findpipe("echo 'a|b' | rev", BUFLEN) == 11);

    printf("Testing Tokenizer\n");
    char* argv[MAXARGS];

    assert(tokenize("ls", argv, MAXARGS) == 1);
    assert(strcmp(argv[0], "ls") == 0);
    assert(argv[1] == NULL);
    free_tokens(argv);

    assert(tokenize("  ls   -l \t /home  ", argv, MAXARGS) == 3);
    assert(strcmp(argv[0], "ls") == 0);
    assert(strcmp(argv[1], "-l") == 0);
    assert(strcmp(argv[2], "/home") == 0);
    assert(argv[3] == NULL);
    free_tokens(argv);

    assert(tokenize("cmd -test \"/home/ensf461/some folder\"", argv, MAXARGS) == 3);
    assert(strcmp(argv[0], "cmd") == 0);
    assert(strcmp(argv[1], "-test") == 0);
    assert(strcmp(argv[2], "/home/ensf461/some folder") == 0);
    free_tokens(argv);

    assert(tokenize("echo 'say \"hi\"'", argv, MAXARGS) == 2);
    assert(strcmp(argv[1], "say \"hi\"") == 0);
    free_tokens(argv);

    assert(tokenize("echo \"a   b\"", argv, MAXARGS) == 2);
    assert(strcmp(argv[1], "a   b") == 0);
    free_tokens(argv);

    assert(tokenize("echo \"\"", argv, MAXARGS) == 2);
    assert(strcmp(argv[1], "") == 0);
    free_tokens(argv);

    assert(tokenize("echo ab\"c d\"e", argv, MAXARGS) == 2);
    assert(strcmp(argv[1], "abc de") == 0);
    free_tokens(argv);

    assert(tokenize("echo \"unterminated", argv, MAXARGS) == -1);
    char* small[3];
    assert(tokenize("a b c", small, 3) == -2);
    assert(tokenize("", argv, MAXARGS) == 0);
    assert(argv[0] == NULL);
    assert(tokenize("    ", argv, MAXARGS) == 0);

    printf("Testing PATH lookup (resolve_path)\n");
    char path[BUFLEN];
    assert(resolve_path("ls", path, BUFLEN) == 0);
    assert(path[0] == '/' && strstr(path, "/ls") != NULL);
    assert(resolve_path("definitely_not_a_cmd", path, BUFLEN) == -1);
    assert(resolve_path("/bin/sh", path, BUFLEN) == 0);
    assert(strcmp(path, "/bin/sh") == 0);
    assert(resolve_path("/no/such/file", path, BUFLEN) == -1);
    assert(resolve_path("/etc", path, BUFLEN) == -1);
    assert(resolve_path("bin/sh", path, BUFLEN) == -1 ||
           resolve_path("bin/sh", path, BUFLEN) == 0);
    assert(resolve_path("", path, BUFLEN) == -1);

    printf("All Tests Passed!\n");
    return 0;
}
