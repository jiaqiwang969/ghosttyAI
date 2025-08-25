// memory_tracker.c - Memory Tracking and Leak Detection
// Author: QA-001 (Test Lead)
// Date: 2025-08-25
// Version: 1.0.0

#include "test_framework.h"
#include <execinfo.h>
#include <pthread.h>

// ============================================================================
// Enhanced Memory Tracking
// ============================================================================

#define MAX_STACK_FRAMES 20
#define MEMORY_MAGIC 0xDEADBEEF
#define MEMORY_FREED 0xFEEDFACE

typedef struct allocation_info {
    void* ptr;
    size_t size;
    uint32_t magic;
    const char* file;
    int line;
    const char* function;
    void* backtrace[MAX_STACK_FRAMES];
    int backtrace_size;
    struct allocation_info* next;
    struct allocation_info* prev;
} allocation_info_t;

static allocation_info_t* g_allocations = NULL;
static pthread_mutex_t g_tracker_mutex = PTHREAD_MUTEX_INITIALIZER;
static size_t g_current_usage = 0;
static size_t g_peak_usage = 0;
static uint64_t g_total_allocations = 0;
static uint64_t g_total_deallocations = 0;

// Track allocation with source location
void* tracked_malloc(size_t size, const char* file, int line, const char* func) {
    void* ptr = malloc(size + sizeof(allocation_info_t));
    if (!ptr) return NULL;
    
    allocation_info_t* info = (allocation_info_t*)ptr;
    info->ptr = (char*)ptr + sizeof(allocation_info_t);
    info->size = size;
    info->magic = MEMORY_MAGIC;
    info->file = file;
    info->line = line;
    info->function = func;
    
    // Capture backtrace
    info->backtrace_size = backtrace(info->backtrace, MAX_STACK_FRAMES);
    
    pthread_mutex_lock(&g_tracker_mutex);
    
    // Add to linked list
    info->next = g_allocations;
    info->prev = NULL;
    if (g_allocations) {
        g_allocations->prev = info;
    }
    g_allocations = info;
    
    // Update statistics
    g_current_usage += size;
    if (g_current_usage > g_peak_usage) {
        g_peak_usage = g_current_usage;
    }
    g_total_allocations++;
    
    pthread_mutex_unlock(&g_tracker_mutex);
    
    return info->ptr;
}

// Track deallocation
void tracked_free(void* ptr) {
    if (!ptr) return;
    
    allocation_info_t* info = (allocation_info_t*)((char*)ptr - sizeof(allocation_info_t));
    
    pthread_mutex_lock(&g_tracker_mutex);
    
    // Verify magic number
    if (info->magic != MEMORY_MAGIC) {
        if (info->magic == MEMORY_FREED) {
            printf("ERROR: Double free detected at %p\n", ptr);
        } else {
            printf("ERROR: Invalid memory magic at %p\n", ptr);
        }
        pthread_mutex_unlock(&g_tracker_mutex);
        abort();
    }
    
    // Mark as freed
    info->magic = MEMORY_FREED;
    
    // Remove from linked list
    if (info->prev) {
        info->prev->next = info->next;
    } else {
        g_allocations = info->next;
    }
    if (info->next) {
        info->next->prev = info->prev;
    }
    
    // Update statistics
    g_current_usage -= info->size;
    g_total_deallocations++;
    
    pthread_mutex_unlock(&g_tracker_mutex);
    
    free(info);
}

// Print memory leak report
void print_leak_report(void) {
    pthread_mutex_lock(&g_tracker_mutex);
    
    printf("\n=== Memory Leak Report ===\n");
    printf("Current usage: %zu bytes\n", g_current_usage);
    printf("Peak usage: %zu bytes\n", g_peak_usage);
    printf("Total allocations: %lu\n", g_total_allocations);
    printf("Total deallocations: %lu\n", g_total_deallocations);
    
    if (g_allocations) {
        printf("\nLeaked allocations:\n");
        printf("%-8s %-10s %-30s %s:%d\n", "Size", "Address", "Function", "File", 0);
        printf("------------------------------------------------------------------------\n");
        
        allocation_info_t* info = g_allocations;
        uint32_t leak_count = 0;
        size_t total_leaked = 0;
        
        while (info) {
            printf("%-8zu 0x%-8lx %-30s %s:%d\n",
                   info->size,
                   (unsigned long)info->ptr,
                   info->function ? info->function : "unknown",
                   info->file ? info->file : "unknown",
                   info->line);
            
            // Print backtrace for first few leaks
            if (leak_count < 3) {
                char** symbols = backtrace_symbols(info->backtrace, info->backtrace_size);
                if (symbols) {
                    printf("  Backtrace:\n");
                    for (int i = 0; i < info->backtrace_size && i < 5; i++) {
                        printf("    %s\n", symbols[i]);
                    }
                    free(symbols);
                }
            }
            
            leak_count++;
            total_leaked += info->size;
            info = info->next;
        }
        
        printf("\nTotal: %u leaks, %zu bytes\n", leak_count, total_leaked);
    } else {
        printf("\nNo memory leaks detected!\n");
    }
    
    pthread_mutex_unlock(&g_tracker_mutex);
}

// Check for buffer overruns
bool check_memory_integrity(void) {
    pthread_mutex_lock(&g_tracker_mutex);
    
    bool integrity_ok = true;
    allocation_info_t* info = g_allocations;
    
    while (info) {
        if (info->magic != MEMORY_MAGIC) {
            printf("ERROR: Corrupted allocation header at %p\n", info->ptr);
            integrity_ok = false;
        }
        info = info->next;
    }
    
    pthread_mutex_unlock(&g_tracker_mutex);
    
    return integrity_ok;
}

// Get memory statistics
void get_memory_stats(size_t* current, size_t* peak, uint64_t* allocs, uint64_t* frees) {
    pthread_mutex_lock(&g_tracker_mutex);
    
    if (current) *current = g_current_usage;
    if (peak) *peak = g_peak_usage;
    if (allocs) *allocs = g_total_allocations;
    if (frees) *frees = g_total_deallocations;
    
    pthread_mutex_unlock(&g_tracker_mutex);
}

// Reset memory tracking
void reset_memory_tracking(void) {
    pthread_mutex_lock(&g_tracker_mutex);
    
    // Free any remaining allocations
    while (g_allocations) {
        allocation_info_t* next = g_allocations->next;
        free(g_allocations);
        g_allocations = next;
    }
    
    g_current_usage = 0;
    g_peak_usage = 0;
    g_total_allocations = 0;
    g_total_deallocations = 0;
    
    pthread_mutex_unlock(&g_tracker_mutex);
}

// ============================================================================
// Memory Stress Testing
// ============================================================================

// Test memory allocation patterns
bool stress_test_memory(size_t max_memory, uint32_t iterations) {
    printf("Starting memory stress test (max: %zu MB, iterations: %u)\n",
           max_memory / (1024 * 1024), iterations);
    
    void** ptrs = calloc(iterations, sizeof(void*));
    if (!ptrs) return false;
    
    bool success = true;
    size_t total_allocated = 0;
    
    // Allocation phase
    for (uint32_t i = 0; i < iterations; i++) {
        size_t size = (rand() % 1024) + 1;  // 1-1024 bytes
        
        if (total_allocated + size > max_memory) {
            break;  // Reached limit
        }
        
        ptrs[i] = tracked_malloc(size, __FILE__, __LINE__, __func__);
        if (!ptrs[i]) {
            printf("Allocation failed at iteration %u\n", i);
            success = false;
            break;
        }
        
        // Fill with pattern to detect corruption
        memset(ptrs[i], i & 0xFF, size);
        total_allocated += size;
    }
    
    printf("Allocated %zu bytes in allocation phase\n", total_allocated);
    
    // Random free/realloc phase
    for (uint32_t i = 0; i < iterations / 2; i++) {
        uint32_t idx = rand() % iterations;
        
        if (ptrs[idx]) {
            tracked_free(ptrs[idx]);
            ptrs[idx] = NULL;
            
            // Reallocate with different size
            size_t new_size = (rand() % 2048) + 1;
            ptrs[idx] = tracked_malloc(new_size, __FILE__, __LINE__, __func__);
            if (ptrs[idx]) {
                memset(ptrs[idx], (idx + 100) & 0xFF, new_size);
            }
        }
    }
    
    // Cleanup phase
    for (uint32_t i = 0; i < iterations; i++) {
        if (ptrs[i]) {
            tracked_free(ptrs[i]);
        }
    }
    
    free(ptrs);
    
    // Check for leaks
    if (!check_memory_integrity()) {
        printf("Memory integrity check failed\n");
        success = false;
    }
    
    size_t current, peak;
    uint64_t allocs, frees;
    get_memory_stats(&current, &peak, &allocs, &frees);
    
    printf("Memory stress test complete:\n");
    printf("  Peak usage: %zu bytes\n", peak);
    printf("  Total allocations: %lu\n", allocs);
    printf("  Total deallocations: %lu\n", frees);
    printf("  Current usage: %zu bytes (should be 0)\n", current);
    
    if (current != 0) {
        printf("  WARNING: Memory leak detected!\n");
        success = false;
    }
    
    return success;
}

// Test for memory fragmentation
void test_memory_fragmentation(void) {
    printf("Testing memory fragmentation...\n");
    
    const size_t sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    const size_t num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    const uint32_t allocs_per_size = 100;
    
    void*** ptrs = calloc(num_sizes, sizeof(void**));
    if (!ptrs) return;
    
    // Allocate in different size classes
    for (size_t i = 0; i < num_sizes; i++) {
        ptrs[i] = calloc(allocs_per_size, sizeof(void*));
        if (!ptrs[i]) continue;
        
        for (uint32_t j = 0; j < allocs_per_size; j++) {
            ptrs[i][j] = tracked_malloc(sizes[i], __FILE__, __LINE__, __func__);
        }
    }
    
    // Free every other allocation to create fragmentation
    for (size_t i = 0; i < num_sizes; i++) {
        if (!ptrs[i]) continue;
        
        for (uint32_t j = 0; j < allocs_per_size; j += 2) {
            if (ptrs[i][j]) {
                tracked_free(ptrs[i][j]);
                ptrs[i][j] = NULL;
            }
        }
    }
    
    // Try to allocate large blocks
    void* large1 = tracked_malloc(1024 * 1024, __FILE__, __LINE__, __func__);
    void* large2 = tracked_malloc(1024 * 1024, __FILE__, __LINE__, __func__);
    
    if (large1 && large2) {
        printf("Successfully allocated large blocks despite fragmentation\n");
    } else {
        printf("Failed to allocate large blocks due to fragmentation\n");
    }
    
    // Cleanup
    if (large1) tracked_free(large1);
    if (large2) tracked_free(large2);
    
    for (size_t i = 0; i < num_sizes; i++) {
        if (!ptrs[i]) continue;
        
        for (uint32_t j = 0; j < allocs_per_size; j++) {
            if (ptrs[i][j]) {
                tracked_free(ptrs[i][j]);
            }
        }
        free(ptrs[i]);
    }
    
    free(ptrs);
}

// Convenience macros for tracked allocation
#define TRACKED_MALLOC(size) tracked_malloc(size, __FILE__, __LINE__, __func__)
#define TRACKED_FREE(ptr) tracked_free(ptr)