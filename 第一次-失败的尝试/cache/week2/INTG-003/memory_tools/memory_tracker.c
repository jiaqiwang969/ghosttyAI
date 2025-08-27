// memory_tracker.c - Custom Memory Tracking for FFI Boundary
// Purpose: Track allocations across C-Zig boundary
// Author: INTG-003 (performance-eng)
// Date: 2025-08-25

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <execinfo.h>
#include <unistd.h>

#define MAX_ALLOCATIONS 10000
#define MAX_STACK_DEPTH 10

typedef struct allocation {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    const char* func;
    void* stack[MAX_STACK_DEPTH];
    int stack_depth;
    uint64_t timestamp_ns;
    int thread_id;
    enum { ALLOC_C, ALLOC_ZIG, ALLOC_FFI } source;
    struct allocation* next;
} allocation_t;

typedef struct {
    allocation_t* allocations[MAX_ALLOCATIONS];
    size_t total_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
    size_t ffi_boundary_crosses;
    pthread_mutex_t lock;
} memory_tracker_t;

static memory_tracker_t g_tracker = {
    .lock = PTHREAD_MUTEX_INITIALIZER
};

// Track allocation
void* track_malloc(size_t size, const char* file, int line, const char* func) {
    void* ptr = malloc(size);
    if (!ptr) return NULL;
    
    pthread_mutex_lock(&g_tracker.lock);
    
    allocation_t* alloc = calloc(1, sizeof(allocation_t));
    alloc->ptr = ptr;
    alloc->size = size;
    alloc->file = file;
    alloc->line = line;
    alloc->func = func;
    alloc->thread_id = pthread_self();
    alloc->source = ALLOC_C;
    
    // Capture stack trace
    alloc->stack_depth = backtrace(alloc->stack, MAX_STACK_DEPTH);
    
    // Update stats
    g_tracker.total_allocated += size;
    g_tracker.current_usage += size;
    if (g_tracker.current_usage > g_tracker.peak_usage) {
        g_tracker.peak_usage = g_tracker.current_usage;
    }
    
    // Add to hash table (simplified)
    uint32_t hash = ((uintptr_t)ptr >> 4) % MAX_ALLOCATIONS;
    alloc->next = g_tracker.allocations[hash];
    g_tracker.allocations[hash] = alloc;
    
    pthread_mutex_unlock(&g_tracker.lock);
    
    return ptr;
}

// Track free
void track_free(void* ptr, const char* file, int line) {
    if (!ptr) return;
    
    pthread_mutex_lock(&g_tracker.lock);
    
    uint32_t hash = ((uintptr_t)ptr >> 4) % MAX_ALLOCATIONS;
    allocation_t* prev = NULL;
    allocation_t* alloc = g_tracker.allocations[hash];
    
    while (alloc) {
        if (alloc->ptr == ptr) {
            // Found it
            if (prev) {
                prev->next = alloc->next;
            } else {
                g_tracker.allocations[hash] = alloc->next;
            }
            
            g_tracker.total_freed += alloc->size;
            g_tracker.current_usage -= alloc->size;
            
            free(alloc);
            break;
        }
        prev = alloc;
        alloc = alloc->next;
    }
    
    pthread_mutex_unlock(&g_tracker.lock);
    
    free(ptr);
}

// Check for FFI boundary crossing
void track_ffi_handoff(void* ptr, enum { FFI_C_TO_ZIG, FFI_ZIG_TO_C } direction) {
    pthread_mutex_lock(&g_tracker.lock);
    
    uint32_t hash = ((uintptr_t)ptr >> 4) % MAX_ALLOCATIONS;
    allocation_t* alloc = g_tracker.allocations[hash];
    
    while (alloc) {
        if (alloc->ptr == ptr) {
            g_tracker.ffi_boundary_crosses++;
            
            // Update source based on direction
            if (direction == FFI_C_TO_ZIG) {
                if (alloc->source == ALLOC_C) {
                    alloc->source = ALLOC_FFI;
                }
            } else {
                if (alloc->source == ALLOC_ZIG) {
                    alloc->source = ALLOC_FFI;
                }
            }
            break;
        }
        alloc = alloc->next;
    }
    
    pthread_mutex_unlock(&g_tracker.lock);
}

// Generate leak report
void memory_tracker_report(FILE* out) {
    pthread_mutex_lock(&g_tracker.lock);
    
    fprintf(out, "\n=== Memory Tracker Report ===\n");
    fprintf(out, "Total Allocated: %zu bytes\n", g_tracker.total_allocated);
    fprintf(out, "Total Freed: %zu bytes\n", g_tracker.total_freed);
    fprintf(out, "Current Usage: %zu bytes\n", g_tracker.current_usage);
    fprintf(out, "Peak Usage: %zu bytes\n", g_tracker.peak_usage);
    fprintf(out, "FFI Boundary Crosses: %zu\n", g_tracker.ffi_boundary_crosses);
    fprintf(out, "Leaked: %zu bytes\n", g_tracker.current_usage);
    
    if (g_tracker.current_usage > 0) {
        fprintf(out, "\n=== Active Allocations ===\n");
        
        for (int i = 0; i < MAX_ALLOCATIONS; i++) {
            allocation_t* alloc = g_tracker.allocations[i];
            while (alloc) {
                fprintf(out, "Leak: %zu bytes at %p\n", alloc->size, alloc->ptr);
                fprintf(out, "  Allocated at: %s:%d in %s()\n", 
                       alloc->file ? alloc->file : "unknown",
                       alloc->line, 
                       alloc->func ? alloc->func : "unknown");
                fprintf(out, "  Source: %s\n", 
                       alloc->source == ALLOC_C ? "C" :
                       alloc->source == ALLOC_ZIG ? "Zig" : "FFI");
                       
                // Print stack trace
                char** symbols = backtrace_symbols(alloc->stack, alloc->stack_depth);
                if (symbols) {
                    fprintf(out, "  Stack trace:\n");
                    for (int j = 0; j < alloc->stack_depth; j++) {
                        fprintf(out, "    %s\n", symbols[j]);
                    }
                    free(symbols);
                }
                
                alloc = alloc->next;
            }
        }
    }
    
    pthread_mutex_unlock(&g_tracker.lock);
}

// Macros for easy tracking
#define TRACK_MALLOC(size) track_malloc(size, __FILE__, __LINE__, __func__)
#define TRACK_FREE(ptr) track_free(ptr, __FILE__, __LINE__)
#define TRACK_FFI_TO_ZIG(ptr) track_ffi_handoff(ptr, FFI_C_TO_ZIG)
#define TRACK_FFI_TO_C(ptr) track_ffi_handoff(ptr, FFI_ZIG_TO_C)

// Cleanup on exit
__attribute__((destructor))
void memory_tracker_cleanup(void) {
    if (g_tracker.current_usage > 0) {
        memory_tracker_report(stderr);
    }
}