#define _POSIX_C_SOURCE 200809L // for open_memstream

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define NUM_CONTEXTS 16
#define BATCH_SIZE 10

typedef struct {
    char op[4];
    int arg;
} Operation;

// One queue of operations per context
typedef struct {
    int ctx;
    Operation *ops;
    size_t count;
    size_t capacity;
    FILE *log;
} Context;

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static void execute(FILE *log, const char *op, int ctx, int arg, int *value);
static void enqueue(Context *context, const char *op, int arg);
static void *run_context(void *data);

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

    Context contexts[NUM_CONTEXTS] = {0};
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

        enqueue(&contexts[ctx], op, arg);
    }

    pthread_t threads[NUM_CONTEXTS];
    for (int i = 0; i < NUM_CONTEXTS; i++) {
        contexts[i].ctx = i;
        contexts[i].log = log;
        if (pthread_create(&threads[i], NULL, run_context, &contexts[i]) != 0) {
            fprintf(stderr, "pthread_create failed\n");
            exit(1);
        }
    }

    for (int i = 0; i < NUM_CONTEXTS; i++) {
        pthread_join(threads[i], NULL);
        free(contexts[i].ops);
    }

    fclose(input);
    fclose(log);
    return 0;
}

static void enqueue(Context *context, const char *op, int arg) {
    if (context->count == context->capacity) {
        size_t capacity = context->capacity == 0 ? 16 : context->capacity * 2;
        Operation *ops = realloc(context->ops, capacity * sizeof(*ops));
        if (ops == NULL) {
            perror("realloc");
            exit(1);
        }
        context->ops = ops;
        context->capacity = capacity;
    }

    Operation *next = &context->ops[context->count++];
    strcpy(next->op, op);
    next->arg = arg;
}

static void *run_context(void *data) {
    Context *context = data;
    int value = 0;
    char *batch = NULL;
    size_t batch_length = 0;
    FILE *buffer = NULL;

    for (size_t i = 0; i < context->count; i++) {
        // Start a new batch of log lines in memory
        if (buffer == NULL) {
            buffer = open_memstream(&batch, &batch_length);
            if (buffer == NULL) {
                perror("open_memstream");
                exit(1);
            }
        }

        execute(buffer, context->ops[i].op, context->ctx,
                context->ops[i].arg, &value);

        // Write after every 10 operations, and when the queue ends
        if ((i + 1) % BATCH_SIZE == 0 || i + 1 == context->count) {
            if (fclose(buffer) != 0) {
                perror("fclose");
                exit(1);
            }
            buffer = NULL;

            pthread_mutex_lock(&log_mutex);
            fwrite(batch, 1, batch_length, context->log);
            pthread_mutex_unlock(&log_mutex);

            free(batch);
        }
    }

    return NULL;
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