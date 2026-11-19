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

void update_tlb(uint32_t vpn, uint32_t pfn)
{
    int entry = -1;

    // Check for an existing mapping.
    for (int i = 0; i < 8; i++)
    {
        if (tlb[i].valid && tlb[i].pid == current_pid && tlb[i].vpn == vpn)
        {
            entry = i;
            break;
        }
    }

    // Find an empty entry or the oldest entry.
    if (entry == -1)
    {
        entry = 0;

        for (int i = 0; i < 8; i++)
        {
            if (!tlb[i].valid)
            {
                entry = i;
                break;
            }

            if (tlb[i].timestamp < tlb[entry].timestamp)
                entry = i;
        }
    }

    tlb[entry].vpn = vpn;
    tlb[entry].pfn = pfn;
    tlb[entry].valid = TRUE;
    tlb[entry].pid = current_pid;
    tlb[entry].timestamp = timestamp;
}

int translate(uint32_t address, uint32_t *physical_address) {
    size_t page_size = (size_t)1 << off_bits;
    uint32_t vpn = address / page_size;
    uint32_t offset = address % page_size;

    for (int i = 0; i < 8; i++)
    {
        if (tlb[i].valid && tlb[i].pid == current_pid && tlb[i].vpn == vpn)
        {
            fprintf(output_file, "Current PID: %d. Translating. Lookup for VPN %u hit in TLB entry %d. PFN is %u\n", current_pid, vpn, i, tlb[i].pfn);

            if (strcmp(strategy, "LRU") == 0)
                tlb[i].timestamp = timestamp;

            *physical_address = tlb[i].pfn * page_size + offset;
            return TRUE;
        }
    }

    fprintf(output_file, "Current PID: %d. Translating. Lookup for VPN %u caused a TLB miss\n", current_pid, vpn);

    if (vpn >= ((size_t)1 << vpn_bits) || !page_tables[current_pid][vpn].valid)
    {
        fprintf(output_file, "Current PID: %d. Translating. Translation for VPN %u not found in page table\n", current_pid, vpn);
        return FALSE;
    }

    uint32_t pfn = page_tables[current_pid][vpn].pfn;

    fprintf(output_file, "Current PID: %d. Translating. Successfully mapped VPN %u to PFN %u\n", current_pid, vpn, pfn);

    update_tlb(vpn, pfn);

    *physical_address = pfn * page_size + offset;
    return TRUE;
}

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

                        else if (strcmp(tokens[0], "load") == 0)
            {
                int reg = -1;

                if (strcmp(tokens[1], "r1") == 0)
                    reg = 0;
                else if (strcmp(tokens[1], "r2") == 0)
                    reg = 1;

                if (reg == -1)
                {
                    fprintf(output_file, "Current PID: %d. Error: invalid register operand %s\n", current_pid, tokens[1]);
                    stop = TRUE;
                }
                else if (tokens[2][0] == '#')
                {
                    uint32_t value = (uint32_t)strtoul(tokens[2] + 1, NULL, 10);

                    registers[current_pid][reg] = value;

                    fprintf(output_file, "Current PID: %d. Loaded immediate %u into register %s\n", current_pid, value, tokens[1]);
                }
                else
                {
                    uint32_t address = (uint32_t)strtoul(tokens[2], NULL, 10);
                    uint32_t physical_address;

                    if (translate(address, &physical_address))
                    {
                        registers[current_pid][reg] = physical_memory[physical_address];

                        fprintf(output_file, "Current PID: %d. Loaded value of location %u (%u) into register %s\n", current_pid, address, registers[current_pid][reg], tokens[1]);
                    }
                    else
                        stop = TRUE;
                }
            }
            else if (strcmp(tokens[0], "add") == 0)
            {
                uint32_t r1 = registers[current_pid][0];
                uint32_t r2 = registers[current_pid][1];

                registers[current_pid][0] = r1 + r2;

                fprintf(output_file, "Current PID: %d. Added contents of registers r1 (%u) and r2 (%u). Result: %u\n", current_pid, r1, r2, registers[current_pid][0]);
            }
            else if (strcmp(tokens[0], "map") == 0)
            {
                uint32_t vpn = (uint32_t)strtoul(tokens[1], NULL, 10);
                uint32_t pfn = (uint32_t)strtoul(tokens[2], NULL, 10);

                page_tables[current_pid][vpn].pfn = pfn;
                page_tables[current_pid][vpn].valid = TRUE;

                update_tlb(vpn, pfn);

                fprintf(output_file, "Current PID: %d. Mapped virtual page number %u to physical frame number %u\n", current_pid, vpn, pfn);
            }
            else if (strcmp(tokens[0], "unmap") == 0)
            {
                uint32_t vpn = (uint32_t)strtoul(tokens[1], NULL, 10);

                page_tables[current_pid][vpn].pfn = 0;
                page_tables[current_pid][vpn].valid = FALSE;

                for (int i = 0; i < 8; i++)
                {
                    if (tlb[i].valid && tlb[i].pid == current_pid && tlb[i].vpn == vpn)
                        memset(&tlb[i], 0, sizeof(TLBEntry));
                }

                fprintf(output_file, "Current PID: %d. Unmapped virtual page number %u\n", current_pid, vpn);
            }
            else if (strcmp(tokens[0], "store") == 0)
            {
                uint32_t address = (uint32_t)strtoul(tokens[1], NULL, 10);
                uint32_t value = 0;
                int reg = -1;

                if (tokens[2][0] == '#')
                    value = (uint32_t)strtoul(tokens[2] + 1, NULL, 10);
                else
                {
                    if (strcmp(tokens[2], "r1") == 0)
                        reg = 0;
                    else if (strcmp(tokens[2], "r2") == 0)
                        reg = 1;

                    if (reg == -1)
                    {
                        fprintf(output_file, "Current PID: %d. Error: invalid register operand %s\n", current_pid, tokens[2]);
                        stop = TRUE;
                    }
                    else
                        value = registers[current_pid][reg];
                }

                if (!stop)
                {
                    uint32_t physical_address;

                    if (translate(address, &physical_address))
                    {
                        physical_memory[physical_address] = value;

                        if (reg == -1)
                            fprintf(output_file, "Current PID: %d. Stored immediate %u into location %u\n", current_pid, value, address);
                        else
                            fprintf(output_file, "Current PID: %d. Stored value of register %s (%u) into location %u\n", current_pid, tokens[2], value, address);
                    }
                    else
                        stop = TRUE;
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