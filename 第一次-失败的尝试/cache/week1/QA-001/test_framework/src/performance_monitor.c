// performance_monitor.c - Performance Monitoring and Benchmarking
// Author: QA-001 (Test Lead)
// Date: 2025-08-25
// Version: 1.0.0

#include "test_framework.h"
#include <unistd.h>
#include <sys/resource.h>

// ============================================================================
// Performance Metrics
// ============================================================================

typedef struct perf_metrics {
    // Timing
    uint64_t start_time_ns;
    uint64_t end_time_ns;
    uint64_t total_time_ns;
    
    // Throughput
    uint64_t operations_completed;
    double operations_per_second;
    uint64_t bytes_processed;
    double throughput_mbps;
    
    // Latency percentiles
    uint64_t* latencies;
    uint32_t latency_count;
    uint32_t latency_capacity;
    uint64_t latency_p50;
    uint64_t latency_p95;
    uint64_t latency_p99;
    uint64_t latency_min;
    uint64_t latency_max;
    double latency_avg;
    
    // Resource usage
    struct rusage rusage_start;
    struct rusage rusage_end;
    double cpu_usage_percent;
    size_t memory_peak_kb;
    
    // Frame metrics
    uint32_t frames_rendered;
    uint32_t frames_dropped;
    double average_fps;
    double min_fps;
    double max_fps;
} perf_metrics_t;

// Create performance metrics tracker
perf_metrics_t* perf_metrics_create(void) {
    perf_metrics_t* metrics = calloc(1, sizeof(perf_metrics_t));
    if (!metrics) return NULL;
    
    metrics->latency_capacity = 10000;
    metrics->latencies = calloc(metrics->latency_capacity, sizeof(uint64_t));
    if (!metrics->latencies) {
        free(metrics);
        return NULL;
    }
    
    metrics->latency_min = UINT64_MAX;
    
    return metrics;
}

// Destroy performance metrics
void perf_metrics_destroy(perf_metrics_t* metrics) {
    if (!metrics) return;
    
    if (metrics->latencies) {
        free(metrics->latencies);
    }
    free(metrics);
}

// Start performance measurement
void perf_metrics_start(perf_metrics_t* metrics) {
    if (!metrics) return;
    
    metrics->start_time_ns = get_time_ns();
    getrusage(RUSAGE_SELF, &metrics->rusage_start);
}

// End performance measurement
void perf_metrics_end(perf_metrics_t* metrics) {
    if (!metrics) return;
    
    metrics->end_time_ns = get_time_ns();
    metrics->total_time_ns = metrics->end_time_ns - metrics->start_time_ns;
    getrusage(RUSAGE_SELF, &metrics->rusage_end);
    
    // Calculate CPU usage
    double user_time = (metrics->rusage_end.ru_utime.tv_sec - metrics->rusage_start.ru_utime.tv_sec) +
                       (metrics->rusage_end.ru_utime.tv_usec - metrics->rusage_start.ru_utime.tv_usec) / 1000000.0;
    double sys_time = (metrics->rusage_end.ru_stime.tv_sec - metrics->rusage_start.ru_stime.tv_sec) +
                      (metrics->rusage_end.ru_stime.tv_usec - metrics->rusage_start.ru_stime.tv_usec) / 1000000.0;
    double wall_time = metrics->total_time_ns / 1000000000.0;
    
    if (wall_time > 0) {
        metrics->cpu_usage_percent = ((user_time + sys_time) / wall_time) * 100.0;
    }
    
    // Memory peak
    metrics->memory_peak_kb = metrics->rusage_end.ru_maxrss;
    
    // Calculate throughput
    if (metrics->total_time_ns > 0) {
        double seconds = metrics->total_time_ns / 1000000000.0;
        metrics->operations_per_second = metrics->operations_completed / seconds;
        metrics->throughput_mbps = (metrics->bytes_processed / (1024.0 * 1024.0)) / seconds;
    }
}

// Record operation latency
void perf_metrics_record_latency(perf_metrics_t* metrics, uint64_t latency_ns) {
    if (!metrics) return;
    
    if (metrics->latency_count >= metrics->latency_capacity) {
        // Resize array
        uint32_t new_capacity = metrics->latency_capacity * 2;
        uint64_t* new_latencies = realloc(metrics->latencies, 
                                         new_capacity * sizeof(uint64_t));
        if (!new_latencies) return;
        
        metrics->latencies = new_latencies;
        metrics->latency_capacity = new_capacity;
    }
    
    metrics->latencies[metrics->latency_count++] = latency_ns;
    
    // Update min/max
    if (latency_ns < metrics->latency_min) {
        metrics->latency_min = latency_ns;
    }
    if (latency_ns > metrics->latency_max) {
        metrics->latency_max = latency_ns;
    }
}

// Compare function for qsort
static int compare_uint64(const void* a, const void* b) {
    uint64_t val_a = *(const uint64_t*)a;
    uint64_t val_b = *(const uint64_t*)b;
    
    if (val_a < val_b) return -1;
    if (val_a > val_b) return 1;
    return 0;
}

// Calculate latency percentiles
void perf_metrics_calculate_percentiles(perf_metrics_t* metrics) {
    if (!metrics || metrics->latency_count == 0) return;
    
    // Sort latencies
    qsort(metrics->latencies, metrics->latency_count, sizeof(uint64_t), compare_uint64);
    
    // Calculate percentiles
    uint32_t p50_idx = metrics->latency_count * 50 / 100;
    uint32_t p95_idx = metrics->latency_count * 95 / 100;
    uint32_t p99_idx = metrics->latency_count * 99 / 100;
    
    metrics->latency_p50 = metrics->latencies[p50_idx];
    metrics->latency_p95 = metrics->latencies[p95_idx];
    metrics->latency_p99 = metrics->latencies[p99_idx];
    
    // Calculate average
    uint64_t sum = 0;
    for (uint32_t i = 0; i < metrics->latency_count; i++) {
        sum += metrics->latencies[i];
    }
    metrics->latency_avg = (double)sum / metrics->latency_count;
}

// Print performance report
void perf_metrics_print_report(perf_metrics_t* metrics) {
    if (!metrics) return;
    
    perf_metrics_calculate_percentiles(metrics);
    
    printf("\n=== Performance Report ===\n");
    
    // Timing
    printf("Duration: %.3f seconds\n", metrics->total_time_ns / 1000000000.0);
    
    // Throughput
    if (metrics->operations_completed > 0) {
        printf("Operations: %lu (%.0f ops/sec)\n",
               metrics->operations_completed,
               metrics->operations_per_second);
    }
    
    if (metrics->bytes_processed > 0) {
        printf("Data processed: %.2f MB (%.2f MB/s)\n",
               metrics->bytes_processed / (1024.0 * 1024.0),
               metrics->throughput_mbps);
    }
    
    // Latency
    if (metrics->latency_count > 0) {
        printf("\nLatency Statistics:\n");
        printf("  Samples: %u\n", metrics->latency_count);
        printf("  Min: %.3f ms\n", metrics->latency_min / 1000000.0);
        printf("  P50: %.3f ms\n", metrics->latency_p50 / 1000000.0);
        printf("  P95: %.3f ms\n", metrics->latency_p95 / 1000000.0);
        printf("  P99: %.3f ms\n", metrics->latency_p99 / 1000000.0);
        printf("  Max: %.3f ms\n", metrics->latency_max / 1000000.0);
        printf("  Avg: %.3f ms\n", metrics->latency_avg / 1000000.0);
    }
    
    // Resource usage
    printf("\nResource Usage:\n");
    printf("  CPU: %.1f%%\n", metrics->cpu_usage_percent);
    printf("  Memory peak: %.2f MB\n", metrics->memory_peak_kb / 1024.0);
    
    // Frame metrics
    if (metrics->frames_rendered > 0) {
        printf("\nFrame Metrics:\n");
        printf("  Frames rendered: %u\n", metrics->frames_rendered);
        printf("  Frames dropped: %u\n", metrics->frames_dropped);
        printf("  Average FPS: %.1f\n", metrics->average_fps);
        printf("  Min FPS: %.1f\n", metrics->min_fps);
        printf("  Max FPS: %.1f\n", metrics->max_fps);
    }
}

// ============================================================================
// Benchmark Functions
// ============================================================================

// Benchmark cell update performance
void benchmark_cell_updates(uint32_t num_cells) {
    printf("Benchmarking cell updates (%u cells)...\n", num_cells);
    
    perf_metrics_t* metrics = perf_metrics_create();
    test_context_t* ctx = test_context_create();
    mock_backend_t* backend = mock_backend_create(ctx);
    
    if (!metrics || !ctx || !backend) {
        printf("Failed to initialize benchmark\n");
        return;
    }
    
    perf_metrics_start(metrics);
    
    // Generate and update cells
    for (uint32_t i = 0; i < num_cells; i++) {
        uint64_t op_start = get_time_ns();
        
        struct tty_ctx* tty_ctx = generate_test_context(
            i % 24,  // Row
            i % 80,  // Column
            1        // Single cell
        );
        
        if (tty_ctx) {
            backend->base.ops->cmd_cell(&backend->base, tty_ctx);
            test_free(tty_ctx->cell_data);
            test_free(tty_ctx);
        }
        
        uint64_t op_end = get_time_ns();
        perf_metrics_record_latency(metrics, op_end - op_start);
        
        metrics->operations_completed++;
        metrics->bytes_processed += sizeof(ui_cell_t);
    }
    
    perf_metrics_end(metrics);
    
    // Calculate cell update rate
    double cells_per_second = calculate_cells_per_second(ctx);
    
    printf("Results:\n");
    printf("  Cells updated: %u\n", num_cells);
    printf("  Time: %.3f seconds\n", metrics->total_time_ns / 1000000000.0);
    printf("  Rate: %.0f cells/second\n", cells_per_second);
    
    if (cells_per_second < PERF_TARGET_CELLS_PER_SEC) {
        printf("  WARNING: Below target of %d cells/second\n", PERF_TARGET_CELLS_PER_SEC);
    } else {
        printf("  SUCCESS: Meets performance target\n");
    }
    
    perf_metrics_print_report(metrics);
    
    // Cleanup
    mock_backend_destroy(backend);
    test_context_destroy(ctx);
    perf_metrics_destroy(metrics);
}

// Benchmark frame aggregation
void benchmark_frame_aggregation(uint32_t num_frames, uint32_t updates_per_frame) {
    printf("Benchmarking frame aggregation (%u frames, %u updates/frame)...\n",
           num_frames, updates_per_frame);
    
    perf_metrics_t* metrics = perf_metrics_create();
    test_context_t* ctx = test_context_create();
    mock_backend_t* backend = mock_backend_create(ctx);
    
    if (!metrics || !ctx || !backend) {
        printf("Failed to initialize benchmark\n");
        return;
    }
    
    perf_metrics_start(metrics);
    
    // Simulate frame generation
    for (uint32_t f = 0; f < num_frames; f++) {
        uint64_t frame_start = get_time_ns();
        
        // Generate updates for this frame
        for (uint32_t u = 0; u < updates_per_frame; u++) {
            struct tty_ctx* tty_ctx = generate_test_context(
                rand() % 24,
                rand() % 80,
                rand() % 10 + 1  // 1-10 cells
            );
            
            if (tty_ctx) {
                backend->base.ops->cmd_cells(&backend->base, tty_ctx);
                test_free(tty_ctx->cell_data);
                test_free(tty_ctx);
            }
        }
        
        // Simulate frame emission
        if (backend->base.on_frame) {
            ui_frame_t frame = {
                .size = sizeof(ui_frame_t),
                .frame_seq = f,
                .timestamp_ns = get_time_ns(),
                .span_count = updates_per_frame,
                .cells_modified = updates_per_frame * 5,  // Average 5 cells per update
                .flags = UI_FRAME_COMPLETE
            };
            
            backend->base.on_frame(&frame, backend);
        }
        
        uint64_t frame_end = get_time_ns();
        uint64_t frame_time = frame_end - frame_start;
        
        perf_metrics_record_latency(metrics, frame_time);
        metrics->frames_rendered++;
        
        // Check if we're maintaining target frame rate
        if (frame_time > TEST_FRAME_INTERVAL_NS) {
            metrics->frames_dropped++;
        }
        
        // Sleep to maintain frame rate
        if (frame_time < TEST_FRAME_INTERVAL_NS) {
            usleep((TEST_FRAME_INTERVAL_NS - frame_time) / 1000);
        }
    }
    
    perf_metrics_end(metrics);
    
    // Calculate frame rate
    metrics->average_fps = calculate_fps(ctx);
    
    printf("Results:\n");
    printf("  Frames generated: %u\n", num_frames);
    printf("  Average FPS: %.1f\n", metrics->average_fps);
    printf("  Frames dropped: %u\n", metrics->frames_dropped);
    
    if (metrics->average_fps < PERF_TARGET_FPS) {
        printf("  WARNING: Below target of %d FPS\n", PERF_TARGET_FPS);
    } else {
        printf("  SUCCESS: Meets FPS target\n");
    }
    
    perf_metrics_print_report(metrics);
    
    // Cleanup
    mock_backend_destroy(backend);
    test_context_destroy(ctx);
    perf_metrics_destroy(metrics);
}

// Benchmark 60 FPS sustained rendering
void benchmark_60fps_sustained(uint32_t duration_seconds) {
    printf("Benchmarking sustained 60 FPS for %u seconds...\n", duration_seconds);
    
    perf_metrics_t* metrics = perf_metrics_create();
    test_context_t* ctx = test_context_create();
    mock_backend_t* backend = mock_backend_create(ctx);
    
    if (!metrics || !ctx || !backend) {
        printf("Failed to initialize benchmark\n");
        return;
    }
    
    uint64_t target_duration_ns = duration_seconds * 1000000000ULL;
    uint64_t frame_interval_ns = 16666667;  // 60 FPS
    uint32_t expected_frames = duration_seconds * 60;
    
    perf_metrics_start(metrics);
    
    uint64_t next_frame_time = get_time_ns() + frame_interval_ns;
    uint32_t frame_count = 0;
    
    while ((get_time_ns() - metrics->start_time_ns) < target_duration_ns) {
        uint64_t frame_start = get_time_ns();
        
        // Simulate realistic frame content
        uint32_t num_updates = rand() % 50 + 10;  // 10-60 updates
        
        for (uint32_t i = 0; i < num_updates; i++) {
            struct tty_ctx* tty_ctx = generate_test_context(
                rand() % 24,
                rand() % 80,
                rand() % 5 + 1
            );
            
            if (tty_ctx) {
                backend->base.ops->cmd_cells(&backend->base, tty_ctx);
                test_free(tty_ctx->cell_data);
                test_free(tty_ctx);
            }
        }
        
        // Emit frame
        if (backend->base.on_frame) {
            ui_frame_t frame = {
                .size = sizeof(ui_frame_t),
                .frame_seq = frame_count++,
                .timestamp_ns = frame_start,
                .span_count = num_updates,
                .cells_modified = num_updates * 3,
                .flags = UI_FRAME_COMPLETE
            };
            
            backend->base.on_frame(&frame, backend);
        }
        
        metrics->frames_rendered++;
        
        // Wait for next frame time
        uint64_t now = get_time_ns();
        if (now < next_frame_time) {
            usleep((next_frame_time - now) / 1000);
        } else {
            // Missed frame deadline
            metrics->frames_dropped++;
        }
        
        next_frame_time += frame_interval_ns;
    }
    
    perf_metrics_end(metrics);
    
    // Calculate actual FPS
    double actual_fps = (double)metrics->frames_rendered / duration_seconds;
    double drop_rate = (double)metrics->frames_dropped / metrics->frames_rendered * 100;
    
    printf("Results:\n");
    printf("  Duration: %u seconds\n", duration_seconds);
    printf("  Expected frames: %u\n", expected_frames);
    printf("  Actual frames: %u\n", metrics->frames_rendered);
    printf("  Dropped frames: %u (%.1f%%)\n", metrics->frames_dropped, drop_rate);
    printf("  Actual FPS: %.1f\n", actual_fps);
    
    if (actual_fps >= 59.0 && drop_rate < 1.0) {  // Allow 1% drop rate
        printf("  SUCCESS: Sustained 60 FPS achieved\n");
    } else {
        printf("  FAILURE: Could not sustain 60 FPS\n");
    }
    
    perf_metrics_print_report(metrics);
    
    // Cleanup
    mock_backend_destroy(backend);
    test_context_destroy(ctx);
    perf_metrics_destroy(metrics);
}