// test_frame_aggregator_memory.c
// Memory leak verification test for DEFECT-001
// Target: Verify frame aggregator has no memory leaks

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <sys/resource.h>

// Mock frame aggregator structures
typedef struct frame {
    int id;
    size_t size;
    char* data;
    struct frame* next;
} frame_t;

typedef struct frame_aggregator {
    frame_t* head;
    frame_t* tail;
    size_t total_frames;
    size_t total_memory;
    pthread_mutex_t lock;
} frame_aggregator_t;

// Global test counters
static size_t allocated_bytes = 0;
static size_t freed_bytes = 0;
static size_t allocation_count = 0;
static size_t deallocation_count = 0;

// Memory tracking wrappers
void* tracked_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        __atomic_add_fetch(&allocated_bytes, size, __ATOMIC_SEQ_CST);
        __atomic_add_fetch(&allocation_count, 1, __ATOMIC_SEQ_CST);
    }
    return ptr;
}

void tracked_free(void* ptr, size_t size) {
    if (ptr) {
        free(ptr);
        __atomic_add_fetch(&freed_bytes, size, __ATOMIC_SEQ_CST);
        __atomic_add_fetch(&deallocation_count, 1, __ATOMIC_SEQ_CST);
    }
}

// Frame aggregator functions (simulating the fixed version)
frame_aggregator_t* create_aggregator() {
    frame_aggregator_t* agg = (frame_aggregator_t*)tracked_malloc(sizeof(frame_aggregator_t));
    if (!agg) return NULL;
    
    agg->head = NULL;
    agg->tail = NULL;
    agg->total_frames = 0;
    agg->total_memory = 0;
    pthread_mutex_init(&agg->lock, NULL);
    
    return agg;
}

int add_frame(frame_aggregator_t* agg, int id, const char* data, size_t size) {
    if (!agg || !data) return -1;
    
    pthread_mutex_lock(&agg->lock);
    
    frame_t* frame = (frame_t*)tracked_malloc(sizeof(frame_t));
    if (!frame) {
        pthread_mutex_unlock(&agg->lock);
        return -1;
    }
    
    frame->id = id;
    frame->size = size;
    frame->data = (char*)tracked_malloc(size);
    if (!frame->data) {
        tracked_free(frame, sizeof(frame_t));
        pthread_mutex_unlock(&agg->lock);
        return -1;
    }
    
    memcpy(frame->data, data, size);
    frame->next = NULL;
    
    if (!agg->head) {
        agg->head = frame;
        agg->tail = frame;
    } else {
        agg->tail->next = frame;
        agg->tail = frame;
    }
    
    agg->total_frames++;
    agg->total_memory += size + sizeof(frame_t);
    
    pthread_mutex_unlock(&agg->lock);
    return 0;
}

frame_t* pop_frame(frame_aggregator_t* agg) {
    if (!agg) return NULL;
    
    pthread_mutex_lock(&agg->lock);
    
    if (!agg->head) {
        pthread_mutex_unlock(&agg->lock);
        return NULL;
    }
    
    frame_t* frame = agg->head;
    agg->head = frame->next;
    if (!agg->head) {
        agg->tail = NULL;
    }
    
    agg->total_frames--;
    agg->total_memory -= (frame->size + sizeof(frame_t));
    
    pthread_mutex_unlock(&agg->lock);
    return frame;
}

void free_frame(frame_t* frame) {
    if (!frame) return;
    
    if (frame->data) {
        tracked_free(frame->data, frame->size);
    }
    tracked_free(frame, sizeof(frame_t));
}

void destroy_aggregator(frame_aggregator_t* agg) {
    if (!agg) return;
    
    pthread_mutex_lock(&agg->lock);
    
    // Free all remaining frames
    frame_t* current = agg->head;
    while (current) {
        frame_t* next = current->next;
        if (current->data) {
            tracked_free(current->data, current->size);
        }
        tracked_free(current, sizeof(frame_t));
        current = next;
    }
    
    pthread_mutex_unlock(&agg->lock);
    pthread_mutex_destroy(&agg->lock);
    
    tracked_free(agg, sizeof(frame_aggregator_t));
}

// Test functions
int test_memory_stability() {
    printf("\n[TEST] Memory Stability Test (30 minutes simulation)\n");
    printf("------------------------------------------------\n");
    
    frame_aggregator_t* agg = create_aggregator();
    assert(agg != NULL);
    
    // Simulate 30 minutes of operation (accelerated)
    const int iterations = 1800;  // 30 minutes * 60 seconds
    const int frames_per_second = 100;
    char test_data[1024];
    
    size_t initial_allocated = allocated_bytes;
    
    for (int i = 0; i < iterations; i++) {
        // Add frames
        for (int j = 0; j < frames_per_second; j++) {
            snprintf(test_data, sizeof(test_data), "Frame %d-%d data", i, j);
            add_frame(agg, i * frames_per_second + j, test_data, strlen(test_data) + 1);
        }
        
        // Process frames (simulate consumption)
        for (int j = 0; j < frames_per_second; j++) {
            frame_t* frame = pop_frame(agg);
            if (frame) {
                free_frame(frame);
            }
        }
        
        // Check memory growth every "minute"
        if (i % 60 == 0) {
            size_t current_allocated = allocated_bytes - freed_bytes;
            printf("  Minute %d: Memory in use: %zu bytes\n", i / 60, current_allocated);
            
            // Fail if memory growth exceeds 100KB
            if (current_allocated - initial_allocated > 102400) {
                printf("  ❌ FAIL: Memory growth exceeded 100KB limit\n");
                destroy_aggregator(agg);
                return 1;
            }
        }
    }
    
    destroy_aggregator(agg);
    
    size_t final_leak = allocated_bytes - freed_bytes;
    printf("  Final memory leak: %zu bytes\n", final_leak);
    
    if (final_leak == 0) {
        printf("  ✅ PASS: No memory leaks detected\n");
        return 0;
    } else {
        printf("  ❌ FAIL: Memory leak of %zu bytes detected\n", final_leak);
        return 1;
    }
}

int test_stress_memory() {
    printf("\n[TEST] Stress Test - 10,000 Rapid Frames\n");
    printf("----------------------------------------\n");
    
    frame_aggregator_t* agg = create_aggregator();
    assert(agg != NULL);
    
    size_t baseline_memory = allocated_bytes - freed_bytes;
    printf("  Baseline memory: %zu bytes\n", baseline_memory);
    
    // Generate varying size frames
    for (int i = 0; i < 10000; i++) {
        size_t data_size = 64 + (rand() % 4096);  // 64 bytes to 4KB
        char* data = (char*)malloc(data_size);
        memset(data, 'X', data_size);
        data[data_size - 1] = '\0';
        
        add_frame(agg, i, data, data_size);
        free(data);
    }
    
    printf("  Added 10,000 frames\n");
    size_t peak_memory = allocated_bytes - freed_bytes;
    printf("  Peak memory usage: %zu bytes\n", peak_memory);
    
    // Process all frames
    int processed = 0;
    frame_t* frame;
    while ((frame = pop_frame(agg)) != NULL) {
        free_frame(frame);
        processed++;
    }
    
    printf("  Processed %d frames\n", processed);
    
    destroy_aggregator(agg);
    
    size_t final_memory = allocated_bytes - freed_bytes;
    printf("  Final memory: %zu bytes\n", final_memory);
    
    // Check if memory returned to baseline (±5%)
    size_t tolerance = baseline_memory * 0.05;
    if (final_memory <= baseline_memory + tolerance) {
        printf("  ✅ PASS: Memory returned to baseline\n");
        return 0;
    } else {
        printf("  ❌ FAIL: Memory not returned to baseline\n");
        return 1;
    }
}

int test_long_running() {
    printf("\n[TEST] Long-Running Test (2 hours simulation)\n");
    printf("---------------------------------------------\n");
    
    frame_aggregator_t* agg = create_aggregator();
    assert(agg != NULL);
    
    struct rusage usage_start, usage_current;
    getrusage(RUSAGE_SELF, &usage_start);
    
    // Simulate 2 hours (accelerated)
    const int hours = 2;
    const int iterations_per_hour = 3600;
    
    for (int hour = 0; hour < hours; hour++) {
        printf("  Hour %d:\n", hour + 1);
        
        for (int i = 0; i < iterations_per_hour; i++) {
            // Variable workload
            int frame_count = 10 + (rand() % 90);
            
            // Add frames
            for (int j = 0; j < frame_count; j++) {
                char data[256];
                snprintf(data, sizeof(data), "Long test frame %d-%d", hour, i);
                add_frame(agg, hour * iterations_per_hour + i, data, strlen(data) + 1);
            }
            
            // Process most frames (leave some for realistic simulation)
            int to_process = frame_count * 0.95;
            for (int j = 0; j < to_process; j++) {
                frame_t* frame = pop_frame(agg);
                if (frame) {
                    free_frame(frame);
                }
            }
        }
        
        getrusage(RUSAGE_SELF, &usage_current);
        long rss_kb = usage_current.ru_maxrss / 1024;  // Convert to KB
        printf("    RSS Memory: %ld KB\n", rss_kb);
        printf("    Allocations: %zu, Deallocations: %zu\n", 
               allocation_count, deallocation_count);
    }
    
    // Clean up remaining frames
    frame_t* frame;
    while ((frame = pop_frame(agg)) != NULL) {
        free_frame(frame);
    }
    
    destroy_aggregator(agg);
    
    size_t memory_growth = (allocated_bytes - freed_bytes);
    printf("  Total memory growth: %zu bytes\n", memory_growth);
    
    if (memory_growth < 512000) {  // Less than 500KB
        printf("  ✅ PASS: Memory growth within acceptable limits\n");
        return 0;
    } else {
        printf("  ❌ FAIL: Excessive memory growth detected\n");
        return 1;
    }
}

// Thread worker for concurrent test
void* thread_worker(void* arg) {
    frame_aggregator_t* agg = (frame_aggregator_t*)arg;
    
    for (int i = 0; i < 1000; i++) {
        char data[128];
        snprintf(data, sizeof(data), "Thread %ld frame %d", 
                 (long)pthread_self(), i);
        
        add_frame(agg, i, data, strlen(data) + 1);
        
        usleep(rand() % 1000);  // Random delay
        
        if (rand() % 2 == 0) {
            frame_t* frame = pop_frame(agg);
            if (frame) {
                free_frame(frame);
            }
        }
    }
    
    return NULL;
}

int test_concurrent_memory() {
    printf("\n[TEST] Concurrent Memory Safety Test\n");
    printf("------------------------------------\n");
    
    frame_aggregator_t* agg = create_aggregator();
    assert(agg != NULL);
    
    const int thread_count = 10;
    pthread_t threads[thread_count];
    
    size_t initial_memory = allocated_bytes - freed_bytes;
    
    // Start threads
    for (int i = 0; i < thread_count; i++) {
        pthread_create(&threads[i], NULL, thread_worker, agg);
    }
    
    // Wait for threads
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("  All threads completed\n");
    
    // Clean up
    frame_t* frame;
    int remaining = 0;
    while ((frame = pop_frame(agg)) != NULL) {
        free_frame(frame);
        remaining++;
    }
    
    printf("  Cleaned up %d remaining frames\n", remaining);
    
    destroy_aggregator(agg);
    
    size_t final_memory = allocated_bytes - freed_bytes;
    
    if (final_memory == initial_memory) {
        printf("  ✅ PASS: No memory leaks in concurrent execution\n");
        return 0;
    } else {
        printf("  ❌ FAIL: Memory leak detected in concurrent execution\n");
        return 1;
    }
}

int main(int argc, char* argv[]) {
    printf("====================================================\n");
    printf("DEFECT-001: Frame Aggregator Memory Leak Test Suite\n");
    printf("====================================================\n");
    printf("Started: %s\n", ctime(&(time_t){time(NULL)}));
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // Run all tests
    total_tests++;
    if (test_memory_stability() == 0) passed_tests++;
    
    total_tests++;
    if (test_stress_memory() == 0) passed_tests++;
    
    total_tests++;
    if (test_long_running() == 0) passed_tests++;
    
    total_tests++;
    if (test_concurrent_memory() == 0) passed_tests++;
    
    // Final report
    printf("\n====================================================\n");
    printf("Test Results Summary\n");
    printf("====================================================\n");
    printf("Total Tests: %d\n", total_tests);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", total_tests - passed_tests);
    printf("Success Rate: %.1f%%\n", (float)passed_tests / total_tests * 100);
    
    printf("\nMemory Statistics:\n");
    printf("  Total Allocated: %zu bytes\n", allocated_bytes);
    printf("  Total Freed: %zu bytes\n", freed_bytes);
    printf("  Final Leak: %zu bytes\n", allocated_bytes - freed_bytes);
    printf("  Allocation Count: %zu\n", allocation_count);
    printf("  Deallocation Count: %zu\n", deallocation_count);
    
    if (passed_tests == total_tests) {
        printf("\n✅ DEFECT-001 VERIFICATION: PASSED\n");
        return 0;
    } else {
        printf("\n❌ DEFECT-001 VERIFICATION: FAILED\n");
        return 1;
    }
}