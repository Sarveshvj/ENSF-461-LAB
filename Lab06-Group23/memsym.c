#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>

#define TRUE 1
#define FALSE 0

// Output file
FILE* output_file;

// TLB replacement strategy (FIFO or LRU)
char* strategy;

typedef struct {
    uint32_t pfn;
    int valid;
} PageTableEntry;

typedef struct {
    uint32_t vpn;
    uint32_t pfn;
    int valid;
    int pid;
    uint32_t timestamp;

} TLBEntry;

int defined = FALSE;
int current_pid = 0;
int off_bits, pfn_bits, vpn_bits;
uint32_t timestamp = 0;

uint32_t* physical_memory = NULL;
PageTableEntry* page_tables[4] = {NULL};
TLBEntry tlb[8] = {0};
uint32_t registers[4][2] = {0};

char** tokenize_input(char* input) {
    char** tokens = NULL;
    char* token = strtok(input, " ");
    int num_tokens = 0;

    while (token != NULL) {
        num_tokens++;
        tokens = realloc(tokens, num_tokens * sizeof(char*));
        tokens[num_tokens - 1] = malloc(strlen(token) + 1);
        strcpy(tokens[num_tokens - 1], token);
        token = strtok(NULL, " ");
    }

    num_tokens++;
    tokens = realloc(tokens, num_tokens * sizeof(char*));
    tokens[num_tokens - 1] = NULL;

    return tokens;
}

int main(int argc, char* argv[]) {
    const char usage[] = "Usage: memsym.out <strategy> <input trace> <output trace>\n";
    char* input_trace;
    char* output_trace;
    char buffer[1024];

    // Parse command line arguments
    if (argc != 4) {
        printf("%s", usage);
        return 1;
    }
    strategy = argv[1];
    input_trace = argv[2];
    output_trace = argv[3];

    // Open input and output files
    FILE* input_file = fopen(input_trace, "r");
    output_file = fopen(output_trace, "w");  

    while ( !feof(input_file) ) {
        // Read input file line by line
        char *rez = fgets(buffer, sizeof(buffer), input_file);
        if ( !rez ) {
            fprintf(stderr, "Reached end of trace. Exiting...\n");
            break;
        } else {
            // Remove endline character
            buffer[strcspn(buffer, "\r\n")] = '\0';
        }
        char** tokens = tokenize_input(buffer);

        // TODO: Implement your memory simulator
                // TODO: Implement your memory simulator
        int stop = FALSE;

        if (tokens[0] != NULL && tokens[0][0] != '%') {
            timestamp++;

            if (strcmp(tokens[0], "define") == 0) {
                if (defined) {
                    fprintf(output_file, "Current PID: %d. Error: multiple calls to define in the same trace\n", current_pid);
                    stop = TRUE;
                } else {
                    off_bits = atoi(tokens[1]);
                    pfn_bits = atoi(tokens[2]);
                    vpn_bits = atoi(tokens[3]);

                    size_t memory_size = (size_t)1 << (off_bits + pfn_bits);
                    size_t page_count = (size_t)1 << vpn_bits;

                    physical_memory = calloc(memory_size, sizeof(uint32_t));

                    for (int i = 0; i < 4; i++) {
                        page_tables[i] = calloc(page_count, sizeof(PageTableEntry));
                    }

                    defined = TRUE;
                    fprintf(output_file, "Current PID: %d. Memory instantiation complete. OFF bits: %d. PFN bits: %d. VPN bits: %d\n", current_pid, off_bits, pfn_bits, vpn_bits);
                }
            } else if (!defined) {
                fprintf(output_file, "Current PID: %d. Error: attempt to execute instruction before define\n", current_pid);
                stop = TRUE;
            } else if (strcmp(tokens[0], "ctxswitch") == 0) {
                int pid = atoi(tokens[1]);

                if (pid < 0 || pid > 3) {
                    fprintf(output_file, "Current PID: %d. Invalid context switch to process %d\n", current_pid, pid);
                    stop = TRUE;
                } else {
                    current_pid = pid;
                    fprintf(output_file, "Current PID: %d. Switched execution context to process: %d\n", current_pid, current_pid);
                }
            }
        }
        // Deallocate tokens
        for (int i = 0; tokens[i] != NULL; i++)
            free(tokens[i]);

        free(tokens);

        if (stop)
            break;
    }
    free(physical_memory);

    for (int i = 0; i < 4; i++)
        free(page_tables[i]);
    // Close input and output files
    fclose(input_file);
    fclose(output_file);

    return 0;
}