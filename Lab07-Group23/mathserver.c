#include <stdio.h>
#include <pthread.h>
#include <string.h>

#define NUM_CONTEXTS 16

static void execute(FILE *log, const char *op, int ctx, int arg, int *value);

int main(int argc, char *argv[]) {
    // Parse command line arguments
    if (argc != 3) {
            printf("Usage: mathserver.out <input trace> <output trace>\n");
            return 1;
        }
    
    // Open input and output files
    FILE *input = fopen(argv[1], "r");
    if (input == NULL) {
        perror(argv[1]);
        return 1;
    }

    FILE *log = fopen(argv[2], "w");
    if (log == NULL) {
        perror(argv[2]);
        fclose(input);
        return 1;
    }

    int values[NUM_CONTEXTS] = {0};
    char line[1024];

    // Read input file line by line
    while (fgets(line, sizeof(line), input) != NULL) {
        // Remove endline character
        line[strcspn(line, "\n")] = '\0';

        // CONTINUE...
        char op[4];
        int ctx;
        int arg = 0;
        int fields = sscanf(line, "%3s %d %d", op, &ctx, &arg);

        if (fields < 2 || ctx < 0 || ctx >= NUM_CONTEXTS) {
            continue;
        }

        execute(log, op, ctx, arg, &values[ctx]);
    }

    fclose(input);
    fclose(log);
    return 0;
}

static int fibonacci(int i){
    if (i <= 1){
        return i;
    }
    return fibonacci(i-1) + fibonacci(i-2);
}

static int is_prime(int n){
    for (int i = 2; i <= n / i; i++){
        if (n % i == 0) {
            return 0;
        }
    }
    return 1;
    
}

static void execute(FILE *log, const char *op, int ctx, int arg, int *value) {
    if (strcmp(op, "set") == 0) {
        *value = arg;
        fprintf(log, "ctx %02d: set to value %d\n", ctx, *value);
        return;
    }

    if (strcmp(op, "fib") == 0) {
        fprintf(log, "ctx %02d: fib (result: %d)\n",
                ctx, fibonacci(*value));
        return;
    }

    if (strcmp(op, "pri") == 0) {
        fprintf(log, "ctx %02d: primes (result: ", ctx);

        const char *separator = "";
        for (int i = 2; i <= *value; i++) {
            if (is_prime(i)) {
                fprintf(log, "%s%d", separator, i);
                separator = ", ";
            }
        }

        fprintf(log, ")\n");
        return;
    }

    if (strcmp(op, "pia") == 0) {
        double sum = 0.0;
        double sign = 1.0;

        for (int i = 0; i < *value; i++) {
            sum += sign / (2.0 * i + 1.0);
            sign = -sign;
        }

        fprintf(log, "ctx %02d: pia (result %.15f)\n", ctx, 4.0 * sum);
        return;
    }

    if (strcmp(op, "add") == 0) {
        *value += arg;
    } else if (strcmp(op, "sub") == 0) {
        *value -= arg;
    } else if (strcmp(op, "mul") == 0) {
        *value *= arg;
    } else if (strcmp(op, "div") == 0 && arg != 0) {
        *value /= arg;
    } else {
        return;
    }

    fprintf(log, "ctx %02d: %s %d (result: %d)\n", ctx, op, arg, *value);
}