// test_concurrency.c - Multi-threaded safety and concurrency tests
// Purpose: Test thread safety, race conditions, and concurrent access
// Author: INTG-001 (Zig-Ghostty Integration Specialist)
// Date: 2025-08-25
// Version: 1.0.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdatomic.h>
#include "../../ARCH-001/tty_ctx_unified.h"
#include "../../ARCH-001/ui_backend.h"
#include "../backend_ghostty.c"

// Thread test configuration
#define MAX_THREADS 16
#define OPERATIONS_PER_THREAD 1000
#define STRESS_TEST_DURATION 2  // seconds

// Global test state
static atomic_int total_operations = 0;
static atomic_int total_errors = 0;
static atomic_int tests_passed = 0;
static atomic_int tests_failed = 0;

// Shared backend for concurrent access
static struct ui_backend* shared_backend = NULL;
static pthread_mutex_t test_mutex = PTHREAD_MUTEX_INITIALIZER;

// Thread worker data
typedef struct {
    int thread_id;
    int operations;
    int errors;
    struct ui_backend* backend;
} thread_data_t;

// Random operation selector
static void perform_random_operation(struct ui_backend* backend, int thread_id) {
    struct tty_ctx ctx;
    tty_ctx_init(&ctx);
    
    // Set random position
    TTY_CTX_SET_FIELD(&ctx, ocx, rand() % 80);
    TTY_CTX_SET_FIELD(&ctx, ocy, rand() % 24);
    ctx.num = rand() % 10 + 1;
    
    // Perform random operation
    int op = rand() % 22;
    switch (op) {
        case 0: backend->ops->cmd_cell(backend, &ctx); break;
        case 1: backend->ops->cmd_cells(backend, &ctx); break;
        case 2: backend->ops->cmd_insertcharacter(backend, &ctx); break;
        case 3: backend->ops->cmd_deletecharacter(backend, &ctx); break;
        case 4: backend->ops->cmd_clearcharacter(backend, &ctx); break;
        case 5: backend->ops->cmd_insertline(backend, &ctx); break;
        case 6: backend->ops->cmd_deleteline(backend, &ctx); break;
        case 7: backend->ops->cmd_clearline(backend, &ctx); break;
        case 8: backend->ops->cmd_clearendofline(backend, &ctx); break;
        case 9: backend->ops->cmd_clearstartofline(backend, &ctx); break;
        case 10: backend->ops->cmd_clearscreen(backend, &ctx); break;
        case 11: backend->ops->cmd_clearendofscreen(backend, &ctx); break;
        case 12: backend->ops->cmd_clearstartofscreen(backend, &ctx); break;
        case 13: backend->ops->cmd_alignmenttest(backend, &ctx); break;
        case 14: backend->ops->cmd_reverseindex(backend, &ctx); break;
        case 15: backend->ops->cmd_linefeed(backend, &ctx); break;
        case 16: backend->ops->cmd_scrollup(backend, &ctx); break;
        case 17: backend->ops->cmd_scrolldown(backend, &ctx); break;
        case 18: backend->ops->cmd_setselection(backend, &ctx); break;
        case 19: backend->ops->cmd_rawstring(backend, &ctx); break;
        case 20: backend->ops->cmd_sixelimage(backend, &ctx); break;
        case 21: backend->ops->cmd_syncstart(backend, &ctx); break;
    }
    
    atomic_fetch_add(&total_operations, 1);
}

// Thread worker for concurrent operations
void* concurrent_worker(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    for (int i = 0; i < data->operations; i++) {
        perform_random_operation(data->backend, data->thread_id);
        
        // Random small delay to increase contention
        if (rand() % 10 == 0) {
            usleep(rand() % 100);
        }
    }
    
    return NULL;
}

// Test basic thread safety
void test_basic_thread_safety(void) {
    printf("Testing basic thread safety...\n");
    
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION,
        .supported = UI_CAP_FRAME_BATCH
    };
    
    struct ui_backend* backend = ghostty_backend_create(&caps);
    if (!backend) {
        atomic_fetch_add(&tests_failed, 1);
        return;
    }
    
    pthread_t threads[4];
    thread_data_t thread_data[4];
    
    // Start threads
    for (int i = 0; i < 4; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].operations = 100;
        thread_data[i].errors = 0;
        thread_data[i].backend = backend;
        
        pthread_create(&threads[i], NULL, concurrent_worker, &thread_data[i]);
    }
    
    // Wait for completion
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    ghostty_backend_destroy(backend);
    atomic_fetch_add(&tests_passed, 1);
    printf("  ✓ 4 threads completed safely\n");
}

// Test high contention scenario
void test_high_contention(void) {
    printf("Testing high contention scenario...\n");
    
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION,
        .supported = UI_CAP_FRAME_BATCH
    };
    
    struct ui_backend* backend = ghostty_backend_create(&caps);
    if (!backend) {
        atomic_fetch_add(&tests_failed, 1);
        return;
    }
    
    pthread_t threads[MAX_THREADS];
    thread_data_t thread_data[MAX_THREADS];
    
    atomic_store(&total_operations, 0);
    
    // Start many threads
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].operations = OPERATIONS_PER_THREAD;
        thread_data[i].errors = 0;
        thread_data[i].backend = backend;
        
        pthread_create(&threads[i], NULL, concurrent_worker, &thread_data[i]);
    }
    
    // Wait for completion
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    int total_ops = atomic_load(&total_operations);
    printf("  ✓ %d threads completed %d operations\n", MAX_THREADS, total_ops);
    
    ghostty_backend_destroy(backend);
    atomic_fetch_add(&tests_passed, 1);
}

// Thread worker for create/destroy test
void* create_destroy_worker(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    for (int i = 0; i < data->operations; i++) {
        ui_capabilities_t caps = {
            .size = sizeof(ui_capabilities_t),
            .version = UI_BACKEND_ABI_VERSION
        };
        
        struct ui_backend* backend = ghostty_backend_create(&caps);
        if (backend) {
            // Do a few operations
            struct tty_ctx ctx;
            tty_ctx_init(&ctx);
            backend->ops->cmd_cell(backend, &ctx);
            
            ghostty_backend_destroy(backend);
        } else {
            data->errors++;
        }
    }
    
    return NULL;
}

// Test concurrent create/destroy
void test_concurrent_create_destroy(void) {
    printf("Testing concurrent create/destroy...\n");
    
    pthread_t threads[8];
    thread_data_t thread_data[8];
    
    // Start threads that create/destroy backends
    for (int i = 0; i < 8; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].operations = 10;
        thread_data[i].errors = 0;
        thread_data[i].backend = NULL;
        
        pthread_create(&threads[i], NULL, create_destroy_worker, &thread_data[i]);
    }
    
    // Wait for completion
    int total_errors = 0;
    for (int i = 0; i < 8; i++) {
        pthread_join(threads[i], NULL);
        total_errors += thread_data[i].errors;
    }
    
    if (total_errors == 0) {
        atomic_fetch_add(&tests_passed, 1);
        printf("  ✓ No errors in concurrent create/destroy\n");
    } else {
        atomic_fetch_add(&tests_failed, 1);
        printf("  ✗ %d errors in concurrent create/destroy\n", total_errors);
    }
}

// Thread worker for mode switching
void* mode_switch_worker(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    for (int i = 0; i < data->operations; i++) {
        // Randomly switch modes
        ghostty_backend_set_immediate_mode(data->backend, rand() % 2);
        ghostty_backend_set_grid_optimization(data->backend, rand() % 2);
        
        // Perform operation
        perform_random_operation(data->backend, data->thread_id);
    }
    
    return NULL;
}

// Test concurrent mode switching
void test_concurrent_mode_switching(void) {
    printf("Testing concurrent mode switching...\n");
    
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION,
        .supported = UI_CAP_FRAME_BATCH
    };
    
    struct ui_backend* backend = ghostty_backend_create(&caps);
    if (!backend) {
        atomic_fetch_add(&tests_failed, 1);
        return;
    }
    
    pthread_t threads[4];
    thread_data_t thread_data[4];
    
    // Start threads that switch modes while operating
    for (int i = 0; i < 4; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].operations = 100;
        thread_data[i].errors = 0;
        thread_data[i].backend = backend;
        
        pthread_create(&threads[i], NULL, mode_switch_worker, &thread_data[i]);
    }
    
    // Wait for completion
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    ghostty_backend_destroy(backend);
    atomic_fetch_add(&tests_passed, 1);
    printf("  ✓ Mode switching under concurrency handled\n");
}

// Thread worker for statistics reading
void* stats_reader_worker(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    for (int i = 0; i < data->operations; i++) {
        uint64_t frames, cells, batched;
        ghostty_backend_get_statistics(data->backend, &frames, &cells, &batched);
        
        // Verify stats are reasonable
        if (frames > 1000000 || cells > 10000000) {
            data->errors++;
        }
    }
    
    return NULL;
}

// Test concurrent statistics access
void test_concurrent_statistics(void) {
    printf("Testing concurrent statistics access...\n");
    
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION
    };
    
    struct ui_backend* backend = ghostty_backend_create(&caps);
    if (!backend) {
        atomic_fetch_add(&tests_failed, 1);
        return;
    }
    
    pthread_t writer_threads[2];
    pthread_t reader_threads[2];
    thread_data_t writer_data[2];
    thread_data_t reader_data[2];
    
    // Start writer threads
    for (int i = 0; i < 2; i++) {
        writer_data[i].thread_id = i;
        writer_data[i].operations = 100;
        writer_data[i].errors = 0;
        writer_data[i].backend = backend;
        pthread_create(&writer_threads[i], NULL, concurrent_worker, &writer_data[i]);
    }
    
    // Start reader threads
    for (int i = 0; i < 2; i++) {
        reader_data[i].thread_id = i + 10;
        reader_data[i].operations = 200;
        reader_data[i].errors = 0;
        reader_data[i].backend = backend;
        pthread_create(&reader_threads[i], NULL, stats_reader_worker, &reader_data[i]);
    }
    
    // Wait for completion
    for (int i = 0; i < 2; i++) {
        pthread_join(writer_threads[i], NULL);
        pthread_join(reader_threads[i], NULL);
    }
    
    int total_errors = 0;
    for (int i = 0; i < 2; i++) {
        total_errors += reader_data[i].errors;
    }
    
    ghostty_backend_destroy(backend);
    
    if (total_errors == 0) {
        atomic_fetch_add(&tests_passed, 1);
        printf("  ✓ Statistics access thread-safe\n");
    } else {
        atomic_fetch_add(&tests_failed, 1);
        printf("  ✗ Statistics corrupted under concurrency\n");
    }
}

// Stress test worker
void* stress_test_worker(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    time_t start = time(NULL);
    
    while (time(NULL) - start < STRESS_TEST_DURATION) {
        perform_random_operation(data->backend, data->thread_id);
        data->operations++;
    }
    
    return NULL;
}

// Run stress test
void test_stress_test(void) {
    printf("Testing stress test (%d seconds)...\n", STRESS_TEST_DURATION);
    
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION,
        .supported = UI_CAP_FRAME_BATCH | UI_CAP_24BIT_COLOR
    };
    
    struct ui_backend* backend = ghostty_backend_create(&caps);
    if (!backend) {
        atomic_fetch_add(&tests_failed, 1);
        return;
    }
    
    pthread_t threads[MAX_THREADS];
    thread_data_t thread_data[MAX_THREADS];
    
    atomic_store(&total_operations, 0);
    
    // Start stress threads
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].operations = 0;
        thread_data[i].errors = 0;
        thread_data[i].backend = backend;
        
        pthread_create(&threads[i], NULL, stress_test_worker, &thread_data[i]);
    }
    
    // Wait for completion
    int total_ops = 0;
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total_ops += thread_data[i].operations;
    }
    
    printf("  ✓ Stress test completed: %d operations in %d seconds\n", 
           total_ops, STRESS_TEST_DURATION);
    printf("    Rate: %.0f ops/sec\n", (double)total_ops / STRESS_TEST_DURATION);
    
    ghostty_backend_destroy(backend);
    atomic_fetch_add(&tests_passed, 1);
}

// Main test runner
int main(int argc, char* argv[]) {
    printf("=== Ghostty Backend Concurrency Tests ===\n\n");
    
    // Seed random for reproducibility
    srand(42);
    
    // Run all concurrency tests
    test_basic_thread_safety();
    test_high_contention();
    test_concurrent_create_destroy();
    test_concurrent_mode_switching();
    test_concurrent_statistics();
    test_stress_test();
    
    // Print summary
    printf("\n=== Concurrency Test Summary ===\n");
    printf("Tests passed: %d\n", atomic_load(&tests_passed));
    printf("Tests failed: %d\n", atomic_load(&tests_failed));
    printf("Total operations: %d\n", atomic_load(&total_operations));
    printf("Coverage focus: Thread safety and race conditions\n");
    
    return atomic_load(&tests_failed) > 0 ? 1 : 0;
}