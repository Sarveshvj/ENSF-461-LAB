#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include "myalloc.h"

int statusno = 0;

static void *arena_start = NULL;
static size_t arena_size = 0;

int myinit(size_t size){
    if (size == 0 || size > MAX_ARENA_SIZE){
        statusno = ERR_BAD_ARGUMENTS;
        return statusno;
    }

    if (arena_start != NULL){
        statusno = ERR_CALL_FAILED;
        return statusno;
    }

    int page_size = getpagesize();
    size_t remainder = size % page_size;

    if (remainder != 0){
        size = size + page_size - remainder;
    }

    if (size > MAX_ARENA_SIZE){
        statusno = ERR_BAD_ARGUMENTS;
        return statusno;
    }

    arena_start = mmap(NULL, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (arena_start == MAP_FAILED){
        arena_start = NULL;
        statusno = ERR_SYSCALL_FAILED;
        return statusno;
    }

    arena_size = size;

    node_t *first = (node_t *)arena_start;
    first->size = arena_size - sizeof(node_t);
    first->is_free = 1;
    first->fwd = NULL;
    first->bwd = NULL;

    statusno = 0;
    return (int)arena_size;
}

int mydestroy(void){
    if (arena_start == NULL){
        statusno = ERR_UNINITIALIZED;
        return statusno;
    }

    if (munmap(arena_start, arena_size) == -1){
        statusno = ERR_SYSCALL_FAILED;
        return statusno;
    }

    arena_start = NULL;
    arena_size = 0;
    statusno = 0;

    return 0;
}

void *myalloc(size_t size){
    if (arena_start == NULL){
        statusno = ERR_UNINITIALIZED;
        return NULL;
    }

    if (size == 0){
        statusno = ERR_BAD_ARGUMENTS;
        return NULL;
    }

    node_t *current = (node_t *)arena_start;

    while (current != NULL){
        if (current->is_free == 1 && current->size >= size){
            size_t remaining = current->size - size;

            if (remaining > sizeof(node_t)){
                char *address = (char *)current;
                address = address + sizeof(node_t) + size;
                node_t *next = (node_t *)address;

                next->size = remaining - sizeof(node_t);
                next->is_free = 1;
                next->fwd = current->fwd;
                next->bwd = current;

                if (next->fwd != NULL){
                    next->fwd->bwd = next;
                }

                current->fwd = next;
                current->size = size;
            }

            current->is_free = 0;
            statusno = 0;
            return (void *)(current + 1);
        }

        current = current->fwd;
    }

    statusno = ERR_OUT_OF_MEMORY;
    return NULL;
}

void myfree(void *ptr){
    if (arena_start == NULL){
        statusno = ERR_UNINITIALIZED;
        return;
    }

    if (ptr == NULL){
        statusno = ERR_BAD_ARGUMENTS;
        return;
    }

    node_t *current = (node_t *)ptr - 1;
    current->is_free = 1;

    // Merge with the next free.
    if (current->fwd != NULL && current->fwd->is_free == 1){
        node_t *next = current->fwd;

        current->size = current->size + sizeof(node_t) + next->size;
        current->fwd = next->fwd;

        if (current->fwd != NULL){
            current->fwd->bwd = current;
        }
    }

    // Merge with the before free chunk.
    if (current->bwd != NULL && current->bwd->is_free == 1){
        node_t *previous = current->bwd;

        previous->size = previous->size + sizeof(node_t) + current->size;
        previous->fwd = current->fwd;

        if (current->fwd != NULL){
            current->fwd->bwd = previous;
        }
    }

    statusno = 0;
}