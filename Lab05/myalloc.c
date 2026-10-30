//Place code here.

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
    if (arena_start != NULL) {
        statusno = ERR_CALL_FAILED;
        return statusno;
    }

    int page_size = getpagesize();
    
    if (size % page_size != 0){
        size = size + page_size - size % page_size;
    }

    if (size > MAX_ARENA_SIZE) {
        statusno = ERR_BAD_ARGUMENTS;
        return statusno;
    }

    arena_start = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,-1,0);

    if (arena_start == MAP_FAILED) {
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
    return (int) arena_size;

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