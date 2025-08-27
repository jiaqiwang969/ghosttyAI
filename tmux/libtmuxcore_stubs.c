/* Simple test implementation for libtmuxcore */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Simple log functions */
void log_open(const char *name) {
    printf("[LOG] Opening log: %s\n", name);
}

void log_close(void) {
    printf("[LOG] Closing log\n");
}

void log_debug(const char *fmt, ...) {
    /* Silent for now */
}

/* Simple memory functions */
void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "xmalloc: out of memory\n");
        exit(1);
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size) {
    ptr = realloc(ptr, size);
    if (ptr == NULL) {
        fprintf(stderr, "xrealloc: out of memory\n");
        exit(1);
    }
    return ptr;
}

void xfree(void *ptr) {
    free(ptr);
}

char *xstrdup(const char *str) {
    char *cp = strdup(str);
    if (cp == NULL) {
        fprintf(stderr, "xstrdup: out of memory\n");
        exit(1);
    }
    return cp;
}