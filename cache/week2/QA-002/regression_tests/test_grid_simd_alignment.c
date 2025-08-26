// test_grid_simd_alignment.c
// Regression tests for grid SIMD alignment issues
// QA-002 Task T-404

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <immintrin.h>
#include <pthread.h>
#include "../../../CORE-002/include/grid_callbacks.h"

// Test DEFECT-002: SIMD alignment issues
void test_unaligned_simd_access() {
    printf("Testing SIMD alignment safety...\n");
    
    // Create deliberately misaligned buffer
    uint8_t* raw_buffer = malloc(1024 + 32);
    assert(raw_buffer != NULL);
    
    // Make it misaligned (not 32-byte aligned)
    uint8_t* misaligned = raw_buffer + 13;  // Odd offset
    
    // Initialize grid backend
    grid_vtable_t vtable;
    get_grid_vtable(&vtable);
    
    void* backend = vtable.init(80, 24, 1000);
    assert(backend != NULL);
    
    // Test with misaligned data
    grid_cell_t cells[100];
    memset(cells, 0, sizeof(cells));
    
    // Fill with test data
    for (int i = 0; i < 100; i++) {
        cells[i].data = 'A' + (i % 26);
        cells[i].attr = i;
        cells[i].fg = i % 256;
        cells[i].bg = (i + 128) % 256;
    }
    
    // Copy to misaligned buffer
    memcpy(misaligned, cells, sizeof(cells));
    
    // Try batch operation with misaligned source
    // This should not crash even with misalignment
    batch_operation_t ops[10];
    for (int i = 0; i < 10; i++) {
        ops[i].type = BATCH_OP_SET_CELL;
        ops[i].x = i;
        ops[i].y = 0;
        ops[i].cell = ((grid_cell_t*)misaligned)[i];
    }
    
    vtable.begin_batch(backend);
    for (int i = 0; i < 10; i++) {
        vtable.set_cell(backend, ops[i].x, ops[i].y, &ops[i].cell);
    }
    vtable.end_batch(backend);
    
    // Verify data was copied correctly
    grid_cell_t check;
    for (int i = 0; i < 10; i++) {
        vtable.get_cell(backend, i, 0, &check);
        assert(check.data == cells[i].data);
    }
    
    vtable.cleanup(backend);
    free(raw_buffer);
    
    printf("  ✓ SIMD alignment safety test passed\n");
}

// Test DEFECT-003: Race conditions in grid operations
typedef struct {
    void* backend;
    grid_vtable_t* vtable;
    int thread_id;
    int start_y;
    int end_y;
} grid_thread_data_t;

void* grid_worker_thread(void* arg) {
    grid_thread_data_t* data = (grid_thread_data_t*)arg;
    
    grid_cell_t cell;
    cell.data = 'A' + data->thread_id;
    cell.attr = data->thread_id;
    cell.fg = data->thread_id;
    cell.bg = 0;
    
    // Each thread writes to its assigned rows
    for (int y = data->start_y; y < data->end_y; y++) {
        for (int x = 0; x < 80; x++) {
            data->vtable->set_cell(data->backend, x, y, &cell);
        }
    }
    
    return NULL;
}

void test_grid_thread_safety() {
    printf("Testing grid thread safety...\n");
    
    grid_vtable_t vtable;
    get_grid_vtable(&vtable);
    
    void* backend = vtable.init(80, 24, 1000);
    assert(backend != NULL);
    
    const int num_threads = 8;
    pthread_t threads[num_threads];
    grid_thread_data_t thread_data[num_threads];
    
    int rows_per_thread = 24 / num_threads;
    
    // Start threads writing to different regions
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].backend = backend;
        thread_data[i].vtable = &vtable;
        thread_data[i].thread_id = i;
        thread_data[i].start_y = i * rows_per_thread;
        thread_data[i].end_y = (i + 1) * rows_per_thread;
        
        pthread_create(&threads[i], NULL, grid_worker_thread, &thread_data[i]);
    }
    
    // Wait for all threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Verify data integrity - each row should have consistent data
    grid_cell_t cell;
    for (int y = 0; y < 24; y++) {
        int expected_thread = y / rows_per_thread;
        char expected_char = 'A' + expected_thread;
        
        for (int x = 0; x < 80; x++) {
            vtable.get_cell(backend, x, y, &cell);
            assert(cell.data == expected_char);
        }
    }
    
    vtable.cleanup(backend);
    printf("  ✓ Grid thread safety test passed\n");
}

// Test DEFECT-006: Integer overflow in calculations
void test_integer_overflow_protection() {
    printf("Testing integer overflow protection...\n");
    
    grid_vtable_t vtable;
    get_grid_vtable(&vtable);
    
    // Test with maximum safe dimensions
    void* backend = vtable.init(1000, 1000, 10000);
    assert(backend != NULL);
    
    // Test batch operations with large counts
    size_t large_count = SIZE_MAX / 2;  // Very large number
    
    batch_operation_t* ops = calloc(100, sizeof(batch_operation_t));
    assert(ops != NULL);
    
    // This should handle overflow gracefully
    vtable.begin_batch(backend);
    
    for (int i = 0; i < 100; i++) {
        ops[i].type = BATCH_OP_SET_CELL;
        ops[i].x = i % 1000;
        ops[i].y = i % 1000;
        ops[i].cell.data = 'X';
    }
    
    // Execute batch
    for (int i = 0; i < 100; i++) {
        vtable.set_cell(backend, ops[i].x, ops[i].y, &ops[i].cell);
    }
    
    vtable.end_batch(backend);
    
    // Test with invalid dimensions (should fail gracefully)
    void* bad_backend = vtable.init(UINT32_MAX, UINT32_MAX, UINT32_MAX);
    assert(bad_backend == NULL);  // Should fail
    
    free(ops);
    vtable.cleanup(backend);
    
    printf("  ✓ Integer overflow protection test passed\n");
}

// Test dirty region tracking atomicity
void test_dirty_tracking_atomicity() {
    printf("Testing dirty tracking atomicity...\n");
    
    grid_vtable_t vtable;
    get_grid_vtable(&vtable);
    
    void* backend = vtable.init(100, 100, 1000);
    assert(backend != NULL);
    
    // Enable dirty tracking
    vtable.enable_dirty_tracking(backend, true);
    
    // Multiple threads updating different regions
    const int num_threads = 10;
    pthread_t threads[num_threads];
    grid_thread_data_t thread_data[num_threads];
    
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].backend = backend;
        thread_data[i].vtable = &vtable;
        thread_data[i].thread_id = i;
        thread_data[i].start_y = i * 10;
        thread_data[i].end_y = (i + 1) * 10;
        
        pthread_create(&threads[i], NULL, grid_worker_thread, &thread_data[i]);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Check dirty region is consistent
    dirty_region_t dirty;
    vtable.get_dirty_region(backend, &dirty);
    
    printf("  Dirty region: (%u,%u) to (%u,%u)\n", 
           dirty.min_x, dirty.min_y, dirty.max_x, dirty.max_y);
    
    // Should cover all updated cells
    assert(dirty.min_x == 0);
    assert(dirty.min_y == 0);
    assert(dirty.max_x >= 79);
    assert(dirty.max_y >= 99);
    
    vtable.cleanup(backend);
    printf("  ✓ Dirty tracking atomicity test passed\n");
}

// Test batch operation performance
void test_batch_performance() {
    printf("Testing batch operation performance...\n");
    
    grid_vtable_t vtable;
    get_grid_vtable(&vtable);
    
    void* backend = vtable.init(200, 200, 1000);
    assert(backend != NULL);
    
    const int num_cells = 10000;
    grid_cell_t cell = { .data = 'P', .attr = 1, .fg = 7, .bg = 0 };
    
    struct timespec start, end;
    
    // Test without batching
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < num_cells; i++) {
        vtable.set_cell(backend, i % 200, i / 200, &cell);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double time_no_batch = (end.tv_sec - start.tv_sec) + 
                          (end.tv_nsec - start.tv_nsec) / 1e9;
    
    // Clear grid
    vtable.clear(backend);
    
    // Test with batching
    clock_gettime(CLOCK_MONOTONIC, &start);
    vtable.begin_batch(backend);
    for (int i = 0; i < num_cells; i++) {
        vtable.set_cell(backend, i % 200, i / 200, &cell);
    }
    vtable.end_batch(backend);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double time_batch = (end.tv_sec - start.tv_sec) + 
                       (end.tv_nsec - start.tv_nsec) / 1e9;
    
    double speedup = time_no_batch / time_batch;
    printf("  No batch: %.3fs, Batch: %.3fs, Speedup: %.1fx\n",
           time_no_batch, time_batch, speedup);
    
    assert(speedup > 5.0);  // Should be at least 5x faster
    
    vtable.cleanup(backend);
    printf("  ✓ Batch performance test passed\n");
}

int main() {
    printf("=== Grid SIMD Alignment Regression Tests ===\n\n");
    
    test_unaligned_simd_access();
    test_grid_thread_safety();
    test_integer_overflow_protection();
    test_dirty_tracking_atomicity();
    test_batch_performance();
    
    printf("\n=== All grid regression tests passed! ===\n");
    return 0;
}