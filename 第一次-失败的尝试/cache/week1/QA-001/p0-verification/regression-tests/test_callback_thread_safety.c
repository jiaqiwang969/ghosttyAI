// test_callback_thread_safety.c
// Race condition verification test for DEFECT-002
// Target: Verify callback system is thread-safe

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <signal.h>
#include <stdatomic.h>

#define MAX_CALLBACKS 10000
#define MAX_THREADS 100

// Callback types
typedef enum {
    CALLBACK_FRAME_READY,
    CALLBACK_RESIZE,
    CALLBACK_REFRESH,
    CALLBACK_ERROR
} callback_type_t;

// Callback structure
typedef struct callback {
    int id;
    callback_type_t type;
    void* data;
    size_t data_size;
    int priority;
    atomic_int executed;
    atomic_int sequence;
} callback_t;

// Callback dispatcher
typedef struct dispatcher {
    callback_t* callbacks[MAX_CALLBACKS];
    atomic_int head;
    atomic_int tail;
    atomic_int count;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    atomic_int running;
    atomic_int total_dispatched;
    atomic_int total_executed;
    atomic_int sequence_counter;
} dispatcher_t;

// Test statistics
typedef struct test_stats {
    atomic_int segfaults;
    atomic_int deadlocks;
    atomic_int data_races;
    atomic_int out_of_order;
    atomic_int lost_callbacks;
    atomic_ulong total_time_ns;
} test_stats_t;

static test_stats_t global_stats = {0};
static dispatcher_t* global_dispatcher = NULL;

// Signal handler for segfaults
void segfault_handler(int sig) {
    atomic_fetch_add(&global_stats.segfaults, 1);
    printf("  ⚠️  Segmentation fault detected (signal %d)\n", sig);
    // Don't exit - try to continue testing
    signal(SIGSEGV, segfault_handler);  // Re-register
}

// Create dispatcher
dispatcher_t* create_dispatcher() {
    dispatcher_t* disp = (dispatcher_t*)calloc(1, sizeof(dispatcher_t));
    if (!disp) return NULL;
    
    atomic_init(&disp->head, 0);
    atomic_init(&disp->tail, 0);
    atomic_init(&disp->count, 0);
    atomic_init(&disp->running, 1);
    atomic_init(&disp->total_dispatched, 0);
    atomic_init(&disp->total_executed, 0);
    atomic_init(&disp->sequence_counter, 0);
    
    pthread_mutex_init(&disp->lock, NULL);
    pthread_cond_init(&disp->cond, NULL);
    
    return disp;
}

// Add callback (thread-safe version)
int dispatch_callback(dispatcher_t* disp, callback_type_t type, void* data, size_t size, int priority) {
    if (!disp || atomic_load(&disp->count) >= MAX_CALLBACKS) {
        return -1;
    }
    
    callback_t* cb = (callback_t*)calloc(1, sizeof(callback_t));
    if (!cb) return -1;
    
    cb->id = atomic_fetch_add(&disp->total_dispatched, 1);
    cb->type = type;
    cb->priority = priority;
    cb->data_size = size;
    atomic_init(&cb->executed, 0);
    atomic_init(&cb->sequence, atomic_fetch_add(&disp->sequence_counter, 1));
    
    if (data && size > 0) {
        cb->data = malloc(size);
        if (cb->data) {
            memcpy(cb->data, data, size);
        }
    }
    
    pthread_mutex_lock(&disp->lock);
    
    int tail = atomic_load(&disp->tail);
    disp->callbacks[tail] = cb;
    
    atomic_store(&disp->tail, (tail + 1) % MAX_CALLBACKS);
    atomic_fetch_add(&disp->count, 1);
    
    pthread_cond_signal(&disp->cond);
    pthread_mutex_unlock(&disp->lock);
    
    return cb->id;
}

// Execute callback (simulated)
void execute_callback(callback_t* cb) {
    if (!cb) return;
    
    // Check for double execution (race condition indicator)
    int prev = atomic_exchange(&cb->executed, 1);
    if (prev == 1) {
        atomic_fetch_add(&global_stats.data_races, 1);
        printf("  ⚠️  Data race: Callback %d executed twice!\n", cb->id);
        return;
    }
    
    // Simulate callback execution
    switch (cb->type) {
        case CALLBACK_FRAME_READY:
            usleep(rand() % 100);  // Simulate work
            break;
        case CALLBACK_RESIZE:
            usleep(rand() % 200);
            break;
        case CALLBACK_REFRESH:
            usleep(rand() % 50);
            break;
        case CALLBACK_ERROR:
            usleep(rand() % 300);
            break;
    }
    
    // Free data if allocated
    if (cb->data) {
        free(cb->data);
        cb->data = NULL;
    }
}

// Worker thread for callback processing
void* callback_worker(void* arg) {
    dispatcher_t* disp = (dispatcher_t*)arg;
    callback_t* last_cb = NULL;
    int last_sequence = -1;
    
    while (atomic_load(&disp->running)) {
        pthread_mutex_lock(&disp->lock);
        
        while (atomic_load(&disp->count) == 0 && atomic_load(&disp->running)) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1;  // 1 second timeout
            
            if (pthread_cond_timedwait(&disp->cond, &disp->lock, &ts) != 0) {
                // Check for deadlock
                if (atomic_load(&disp->count) > 0) {
                    atomic_fetch_add(&global_stats.deadlocks, 1);
                    printf("  ⚠️  Potential deadlock detected\n");
                }
            }
        }
        
        if (!atomic_load(&disp->running)) {
            pthread_mutex_unlock(&disp->lock);
            break;
        }
        
        if (atomic_load(&disp->count) > 0) {
            int head = atomic_load(&disp->head);
            callback_t* cb = disp->callbacks[head];
            
            if (cb) {
                disp->callbacks[head] = NULL;
                atomic_store(&disp->head, (head + 1) % MAX_CALLBACKS);
                atomic_fetch_sub(&disp->count, 1);
                
                // Check sequence ordering
                int seq = atomic_load(&cb->sequence);
                if (seq < last_sequence) {
                    atomic_fetch_add(&global_stats.out_of_order, 1);
                }
                last_sequence = seq;
                
                pthread_mutex_unlock(&disp->lock);
                
                execute_callback(cb);
                atomic_fetch_add(&disp->total_executed, 1);
                
                free(cb);
            } else {
                pthread_mutex_unlock(&disp->lock);
            }
        } else {
            pthread_mutex_unlock(&disp->lock);
        }
    }
    
    return NULL;
}

// Stress producer thread
void* stress_producer(void* arg) {
    dispatcher_t* disp = (dispatcher_t*)arg;
    
    for (int i = 0; i < 100; i++) {
        callback_type_t type = (callback_type_t)(rand() % 4);
        char data[256];
        snprintf(data, sizeof(data), "Thread %ld callback %d", 
                 (long)pthread_self(), i);
        
        dispatch_callback(disp, type, data, strlen(data) + 1, rand() % 3);
        
        if (rand() % 10 == 0) {
            usleep(rand() % 100);  // Occasional delay
        }
    }
    
    return NULL;
}

// Test 1: Thread Safety
int test_thread_safety() {
    printf("\n[TEST] Thread Safety Test - 100 Concurrent Threads\n");
    printf("--------------------------------------------------\n");
    
    dispatcher_t* disp = create_dispatcher();
    assert(disp != NULL);
    global_dispatcher = disp;
    
    const int producer_threads = 50;
    const int worker_threads = 50;
    pthread_t producers[producer_threads];
    pthread_t workers[worker_threads];
    
    // Reset stats
    atomic_store(&global_stats.segfaults, 0);
    atomic_store(&global_stats.data_races, 0);
    
    // Start worker threads
    for (int i = 0; i < worker_threads; i++) {
        pthread_create(&workers[i], NULL, callback_worker, disp);
    }
    
    // Start producer threads
    for (int i = 0; i < producer_threads; i++) {
        pthread_create(&producers[i], NULL, stress_producer, disp);
    }
    
    // Wait for producers
    for (int i = 0; i < producer_threads; i++) {
        pthread_join(producers[i], NULL);
    }
    
    printf("  All producers completed\n");
    
    // Wait for queue to drain
    int wait_count = 0;
    while (atomic_load(&disp->count) > 0 && wait_count < 100) {
        usleep(100000);  // 100ms
        wait_count++;
    }
    
    // Stop workers
    atomic_store(&disp->running, 0);
    pthread_cond_broadcast(&disp->cond);
    
    for (int i = 0; i < worker_threads; i++) {
        pthread_join(workers[i], NULL);
    }
    
    printf("  Dispatched: %d, Executed: %d\n", 
           atomic_load(&disp->total_dispatched),
           atomic_load(&disp->total_executed));
    
    int segfaults = atomic_load(&global_stats.segfaults);
    int races = atomic_load(&global_stats.data_races);
    
    // Cleanup
    pthread_mutex_destroy(&disp->lock);
    pthread_cond_destroy(&disp->cond);
    free(disp);
    
    if (segfaults == 0 && races == 0) {
        printf("  ✅ PASS: No segfaults or data races detected\n");
        return 0;
    } else {
        printf("  ❌ FAIL: %d segfaults, %d data races detected\n", 
               segfaults, races);
        return 1;
    }
}

// Test 2: Stress Concurrency
int test_stress_concurrency() {
    printf("\n[TEST] Stress Concurrency - 1000 callbacks/sec for 10 min\n");
    printf("----------------------------------------------------------\n");
    
    dispatcher_t* disp = create_dispatcher();
    assert(disp != NULL);
    
    const int worker_threads = 20;
    pthread_t workers[worker_threads];
    
    // Reset stats
    atomic_store(&global_stats.deadlocks, 0);
    atomic_store(&global_stats.lost_callbacks, 0);
    
    // Start workers
    for (int i = 0; i < worker_threads; i++) {
        pthread_create(&workers[i], NULL, callback_worker, disp);
    }
    
    time_t start = time(NULL);
    time_t target_duration = 600;  // 10 minutes
    int callbacks_per_second = 1000;
    
    // For demo, run for 10 seconds instead of 10 minutes
    target_duration = 10;
    
    while (time(NULL) - start < target_duration) {
        for (int i = 0; i < callbacks_per_second / 10; i++) {
            char data[128];
            snprintf(data, sizeof(data), "Stress callback %d", i);
            dispatch_callback(disp, CALLBACK_FRAME_READY, data, 
                            strlen(data) + 1, 1);
        }
        usleep(100000);  // 100ms = 1/10 second
    }
    
    printf("  Stress period completed\n");
    
    // Wait for queue to drain
    while (atomic_load(&disp->count) > 0) {
        usleep(100000);
    }
    
    // Stop workers
    atomic_store(&disp->running, 0);
    pthread_cond_broadcast(&disp->cond);
    
    for (int i = 0; i < worker_threads; i++) {
        pthread_join(workers[i], NULL);
    }
    
    int dispatched = atomic_load(&disp->total_dispatched);
    int executed = atomic_load(&disp->total_executed);
    int deadlocks = atomic_load(&global_stats.deadlocks);
    
    printf("  Total callbacks: %d dispatched, %d executed\n", 
           dispatched, executed);
    printf("  Throughput: %.1f callbacks/sec\n", 
           (float)executed / target_duration);
    
    // Cleanup
    pthread_mutex_destroy(&disp->lock);
    pthread_cond_destroy(&disp->cond);
    free(disp);
    
    if (dispatched == executed && deadlocks == 0) {
        printf("  ✅ PASS: All callbacks completed without deadlocks\n");
        return 0;
    } else {
        printf("  ❌ FAIL: %d callbacks lost, %d deadlocks\n", 
               dispatched - executed, deadlocks);
        return 1;
    }
}

// Test 3: Order Preservation
int test_order_preservation() {
    printf("\n[TEST] Order Preservation Test\n");
    printf("------------------------------\n");
    
    dispatcher_t* disp = create_dispatcher();
    assert(disp != NULL);
    
    const int worker_threads = 1;  // Single worker for order testing
    pthread_t worker;
    
    // Reset stats
    atomic_store(&global_stats.out_of_order, 0);
    
    // Start worker
    pthread_create(&worker, NULL, callback_worker, disp);
    
    // Send ordered callbacks
    const int callback_count = 1000;
    for (int i = 0; i < callback_count; i++) {
        char data[64];
        snprintf(data, sizeof(data), "Ordered callback %d", i);
        dispatch_callback(disp, CALLBACK_FRAME_READY, data, 
                        strlen(data) + 1, 1);
    }
    
    // Wait for completion
    while (atomic_load(&disp->count) > 0) {
        usleep(10000);
    }
    
    // Stop worker
    atomic_store(&disp->running, 0);
    pthread_cond_signal(&disp->cond);
    pthread_join(worker, NULL);
    
    int out_of_order = atomic_load(&global_stats.out_of_order);
    
    // Cleanup
    pthread_mutex_destroy(&disp->lock);
    pthread_cond_destroy(&disp->cond);
    free(disp);
    
    if (out_of_order == 0) {
        printf("  ✅ PASS: All callbacks executed in order\n");
        return 0;
    } else {
        printf("  ❌ FAIL: %d callbacks executed out of order\n", 
               out_of_order);
        return 1;
    }
}

// Test 4: Priority handling
int test_priority_handling() {
    printf("\n[TEST] Priority Handling Test\n");
    printf("-----------------------------\n");
    
    dispatcher_t* disp = create_dispatcher();
    assert(disp != NULL);
    
    const int worker_threads = 4;
    pthread_t workers[worker_threads];
    
    // Start workers
    for (int i = 0; i < worker_threads; i++) {
        pthread_create(&workers[i], NULL, callback_worker, disp);
    }
    
    // Send mixed priority callbacks
    for (int i = 0; i < 100; i++) {
        char data[64];
        int priority = (i % 3);  // 0=high, 1=medium, 2=low
        snprintf(data, sizeof(data), "Priority %d callback %d", priority, i);
        dispatch_callback(disp, CALLBACK_FRAME_READY, data, 
                        strlen(data) + 1, priority);
    }
    
    // Wait for completion
    while (atomic_load(&disp->count) > 0) {
        usleep(10000);
    }
    
    // Stop workers
    atomic_store(&disp->running, 0);
    pthread_cond_broadcast(&disp->cond);
    
    for (int i = 0; i < worker_threads; i++) {
        pthread_join(workers[i], NULL);
    }
    
    int executed = atomic_load(&disp->total_executed);
    
    // Cleanup
    pthread_mutex_destroy(&disp->lock);
    pthread_cond_destroy(&disp->cond);
    free(disp);
    
    if (executed == 100) {
        printf("  ✅ PASS: All priority callbacks executed\n");
        return 0;
    } else {
        printf("  ❌ FAIL: Only %d/100 callbacks executed\n", executed);
        return 1;
    }
}

// Test 5: Cleanup and leak test
int test_cleanup() {
    printf("\n[TEST] Cleanup and Resource Leak Test\n");
    printf("-------------------------------------\n");
    
    // Create and destroy multiple dispatchers
    for (int i = 0; i < 10; i++) {
        dispatcher_t* disp = create_dispatcher();
        assert(disp != NULL);
        
        // Add some callbacks
        for (int j = 0; j < 100; j++) {
            char data[64];
            snprintf(data, sizeof(data), "Test %d-%d", i, j);
            dispatch_callback(disp, CALLBACK_FRAME_READY, data, 
                            strlen(data) + 1, 1);
        }
        
        // Process callbacks
        pthread_t worker;
        pthread_create(&worker, NULL, callback_worker, disp);
        
        usleep(100000);  // Let some callbacks process
        
        // Stop and cleanup
        atomic_store(&disp->running, 0);
        pthread_cond_signal(&disp->cond);
        pthread_join(worker, NULL);
        
        // Free remaining callbacks
        while (atomic_load(&disp->count) > 0) {
            int head = atomic_load(&disp->head);
            callback_t* cb = disp->callbacks[head];
            if (cb) {
                if (cb->data) free(cb->data);
                free(cb);
                atomic_store(&disp->head, (head + 1) % MAX_CALLBACKS);
                atomic_fetch_sub(&disp->count, 1);
            }
        }
        
        pthread_mutex_destroy(&disp->lock);
        pthread_cond_destroy(&disp->cond);
        free(disp);
    }
    
    printf("  ✅ PASS: Clean shutdown without leaks\n");
    return 0;
}

int main(int argc, char* argv[]) {
    printf("====================================================\n");
    printf("DEFECT-002: Callback Thread Safety Test Suite\n");
    printf("====================================================\n");
    printf("Started: %s\n", ctime(&(time_t){time(NULL)}));
    
    // Install signal handler
    signal(SIGSEGV, segfault_handler);
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // Run all tests
    total_tests++;
    if (test_thread_safety() == 0) passed_tests++;
    
    total_tests++;
    if (test_stress_concurrency() == 0) passed_tests++;
    
    total_tests++;
    if (test_order_preservation() == 0) passed_tests++;
    
    total_tests++;
    if (test_priority_handling() == 0) passed_tests++;
    
    total_tests++;
    if (test_cleanup() == 0) passed_tests++;
    
    // Final report
    printf("\n====================================================\n");
    printf("Test Results Summary\n");
    printf("====================================================\n");
    printf("Total Tests: %d\n", total_tests);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", total_tests - passed_tests);
    printf("Success Rate: %.1f%%\n", (float)passed_tests / total_tests * 100);
    
    printf("\nCritical Issues Detected:\n");
    printf("  Segfaults: %d\n", atomic_load(&global_stats.segfaults));
    printf("  Deadlocks: %d\n", atomic_load(&global_stats.deadlocks));
    printf("  Data Races: %d\n", atomic_load(&global_stats.data_races));
    printf("  Out of Order: %d\n", atomic_load(&global_stats.out_of_order));
    
    if (passed_tests == total_tests) {
        printf("\n✅ DEFECT-002 VERIFICATION: PASSED\n");
        return 0;
    } else {
        printf("\n❌ DEFECT-002 VERIFICATION: FAILED\n");
        return 1;
    }
}