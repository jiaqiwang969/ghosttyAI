# Frame Batching and Aggregation Design

## Overview
This document describes the frame-level batching system that aggregates multiple tmux grid updates into coherent frames, reducing callback overhead and enabling smooth 60 FPS rendering.

## Core Concepts

### Frame Definition
A frame represents a complete, coherent state update that can be rendered atomically:

```c
typedef struct {
    uint32_t size;              // Structure size for ABI compatibility
    uint64_t frame_seq;         // Monotonically increasing sequence number
    uint64_t timestamp_ns;      // Frame generation timestamp (monotonic clock)
    uint32_t pane_id;          // Target pane identifier
    uint32_t span_count;       // Number of dirty spans in this frame
    const tmc_span_t* spans;   // Array of dirty regions
    tmc_frame_flags_t flags;   // Frame metadata flags
} tmc_frame_t;

typedef enum {
    TMC_FRAME_COMPLETE = 1 << 0,    // Frame contains all changes
    TMC_FRAME_PARTIAL  = 1 << 1,    // More updates coming
    TMC_FRAME_URGENT   = 1 << 2,    // Bypass batching (e.g., cursor)
    TMC_FRAME_DROPPED  = 1 << 3,    // Previous frames were dropped
    TMC_FRAME_SNAPSHOT = 1 << 4,    // Full grid snapshot
} tmc_frame_flags_t;
```

## Batching Strategy

### Time-Based Aggregation
```c
// Frame aggregator state
typedef struct {
    uint64_t last_frame_time;      // Last frame emission time
    uint64_t frame_interval_ns;    // Target interval (16.67ms for 60 FPS)
    uint64_t max_latency_ns;       // Maximum buffering time (8ms)
    
    // Accumulation buffer
    tmc_span_list_t pending_spans;
    uint32_t pending_count;
    
    // Statistics
    uint64_t frames_emitted;
    uint64_t spans_merged;
    uint64_t updates_batched;
} frame_aggregator_t;
```

### Aggregation Algorithm
```c
void aggregate_update(frame_aggregator_t* agg, const tmc_grid_update_t* update) {
    uint64_t now = get_monotonic_time_ns();
    uint64_t elapsed = now - agg->last_frame_time;
    
    // Add update to pending spans
    add_spans_to_list(&agg->pending_spans, update->spans, update->count);
    agg->pending_count++;
    
    // Determine if we should emit a frame
    bool should_emit = false;
    
    if (update->flags & TMC_UPDATE_URGENT) {
        // Urgent updates (cursor movement, bell) emit immediately
        should_emit = true;
    } else if (elapsed >= agg->frame_interval_ns) {
        // Time for next frame (vsync aligned)
        should_emit = true;
    } else if (elapsed >= agg->max_latency_ns && agg->pending_count > 0) {
        // Maximum latency reached
        should_emit = true;
    } else if (agg->pending_spans.memory_usage > MAX_BUFFER_SIZE) {
        // Buffer size limit reached
        should_emit = true;
    }
    
    if (should_emit) {
        emit_frame(agg);
    }
}
```

## Span Merging and Optimization

### Adjacent Span Merging
```c
typedef struct {
    uint32_t row;
    uint32_t col_start;
    uint32_t col_end;
    uint32_t flags;
    tmc_cell_t* cells;  // Shared cell data
} tmc_span_t;

// Merge adjacent spans on the same row
void merge_spans(tmc_span_list_t* list) {
    for (size_t i = 0; i < list->count - 1; i++) {
        tmc_span_t* current = &list->spans[i];
        tmc_span_t* next = &list->spans[i + 1];
        
        if (current->row == next->row &&
            current->col_end == next->col_start &&
            compatible_attributes(current, next)) {
            // Merge spans
            current->col_end = next->col_end;
            // Remove next span
            memmove(next, next + 1, (list->count - i - 2) * sizeof(tmc_span_t));
            list->count--;
            i--; // Recheck current position
        }
    }
}
```

### Dirty Rectangle Tracking
```c
typedef struct {
    uint32_t min_row, max_row;
    uint32_t min_col, max_col;
    bool valid;
} dirty_rect_t;

// Track dirty regions for optimal updates
typedef struct {
    dirty_rect_t rects[MAX_DIRTY_RECTS];
    uint32_t rect_count;
    bool full_refresh;  // Entire screen is dirty
} dirty_tracker_t;

// Coalesce overlapping rectangles
void coalesce_dirty_rects(dirty_tracker_t* tracker) {
    if (tracker->rect_count > COALESCE_THRESHOLD) {
        // Too many small updates, switch to full refresh
        tracker->full_refresh = true;
        tracker->rect_count = 0;
        return;
    }
    
    // Merge overlapping rectangles
    for (size_t i = 0; i < tracker->rect_count - 1; i++) {
        for (size_t j = i + 1; j < tracker->rect_count; j++) {
            if (rects_overlap(&tracker->rects[i], &tracker->rects[j])) {
                merge_rects(&tracker->rects[i], &tracker->rects[j]);
                // Remove j-th rectangle
                tracker->rects[j] = tracker->rects[--tracker->rect_count];
                j--;
            }
        }
    }
}
```

## Vsync Synchronization

### Platform-Specific Vsync
```c
// macOS - CVDisplayLink
typedef struct {
    CVDisplayLinkRef display_link;
    dispatch_queue_t frame_queue;
    frame_aggregator_t* aggregator;
} vsync_timer_mac_t;

static CVReturn display_link_callback(CVDisplayLinkRef displayLink,
                                     const CVTimeStamp* now,
                                     const CVTimeStamp* outputTime,
                                     CVOptionFlags flagsIn,
                                     CVOptionFlags* flagsOut,
                                     void* context) {
    vsync_timer_mac_t* timer = context;
    dispatch_async(timer->frame_queue, ^{
        check_and_emit_frame(timer->aggregator);
    });
    return kCVReturnSuccess;
}

// Linux - DRM vblank events or timerfd fallback
typedef struct {
    int drm_fd;           // DRM device for vblank events
    int timer_fd;         // Fallback timer
    uint64_t refresh_rate_hz;
    frame_aggregator_t* aggregator;
} vsync_timer_linux_t;

// Windows - DwmGetCompositionTimingInfo or D3D present
typedef struct {
    HANDLE timer_queue;
    HANDLE timer;
    frame_aggregator_t* aggregator;
} vsync_timer_win_t;
```

### Adaptive Frame Rate
```c
typedef struct {
    uint64_t target_fps;         // Desired frame rate (30/60/120)
    uint64_t min_fps;           // Minimum acceptable (15 FPS)
    uint64_t current_fps;       // Measured frame rate
    
    // Adaptive parameters
    uint64_t frame_budget_ns;   // Time budget per frame
    uint64_t render_time_ns;    // Last frame render time
    float load_factor;          // System load (0.0-1.0)
    
    // History for averaging
    uint64_t frame_times[16];
    uint32_t frame_time_index;
} adaptive_fps_t;

void update_frame_rate(adaptive_fps_t* fps, uint64_t render_time) {
    fps->render_time_ns = render_time;
    fps->frame_times[fps->frame_time_index++ % 16] = render_time;
    
    // Calculate average render time
    uint64_t avg_render = calculate_average(fps->frame_times, 16);
    
    // Adjust target FPS based on performance
    if (avg_render > fps->frame_budget_ns * 0.9) {
        // Can't keep up, reduce frame rate
        if (fps->target_fps > fps->min_fps) {
            fps->target_fps = fps->target_fps * 3 / 4;  // Reduce by 25%
        }
    } else if (avg_render < fps->frame_budget_ns * 0.5) {
        // Have headroom, can increase frame rate
        if (fps->target_fps < 120) {
            fps->target_fps = MIN(fps->target_fps * 4 / 3, 120);
        }
    }
    
    fps->frame_budget_ns = 1000000000ULL / fps->target_fps;
}
```

## Frame Sequencing and Recovery

### Sequence Number Management
```c
typedef struct {
    uint64_t next_seq;           // Next sequence to emit
    uint64_t last_acked_seq;     // Last acknowledged by UI
    uint64_t pending_seqs[64];   // Ring buffer of pending
    uint32_t pending_count;
    
    // Gap detection
    uint64_t missing_seqs[16];   // Detected gaps
    uint32_t missing_count;
} sequence_tracker_t;

// Check for gaps and request retransmission
void check_sequence_gaps(sequence_tracker_t* tracker, uint64_t received_seq) {
    if (received_seq > tracker->last_acked_seq + 1) {
        // Gap detected
        for (uint64_t seq = tracker->last_acked_seq + 1; seq < received_seq; seq++) {
            if (tracker->missing_count < 16) {
                tracker->missing_seqs[tracker->missing_count++] = seq;
            }
        }
        // Request retransmission or snapshot
        request_recovery(tracker);
    }
    tracker->last_acked_seq = MAX(tracker->last_acked_seq, received_seq);
}
```

### Frame Dropping and Recovery
```c
typedef struct {
    uint32_t max_pending_frames;    // Maximum frames to buffer
    uint32_t drop_threshold;        // Start dropping at this level
    uint32_t snapshot_threshold;     // Force snapshot at this level
    
    // Statistics
    uint64_t frames_dropped;
    uint64_t snapshots_forced;
    uint64_t recovery_time_ms;
} drop_policy_t;

tmc_frame_action_t evaluate_frame_pressure(drop_policy_t* policy, 
                                          uint32_t pending_count) {
    if (pending_count >= policy->snapshot_threshold) {
        // Too far behind, send full snapshot
        policy->snapshots_forced++;
        return TMC_ACTION_SNAPSHOT;
    } else if (pending_count >= policy->drop_threshold) {
        // Start dropping non-essential frames
        policy->frames_dropped++;
        return TMC_ACTION_DROP_OLD;
    }
    return TMC_ACTION_EMIT;
}
```

## UTF-8 Line Optimization

### Line-Based Updates
```c
typedef struct {
    uint32_t row;
    const char* utf8_content;       // UTF-8 encoded line content
    uint32_t byte_length;           // Total bytes in UTF-8 string
    const uint32_t* cell_offsets;  // Byte offset for each cell
    uint32_t cell_count;           // Number of cells
    const tmc_attr_t* attributes;  // Parallel array of attributes
} tmc_line_update_t;

// Emit entire lines when efficient
void emit_line_update(uint32_t row, const grid_line_t* line) {
    tmc_line_update_t update = {
        .row = row,
        .utf8_content = line->utf8_buffer,
        .byte_length = line->utf8_length,
        .cell_offsets = line->cell_map,
        .cell_count = line->cell_count,
        .attributes = line->attributes
    };
    
    // Check if line update is more efficient than spans
    uint32_t span_cost = calculate_span_cost(line);
    uint32_t line_cost = sizeof(update) + update.byte_length;
    
    if (line_cost < span_cost) {
        emit_callback_line(&update);
    } else {
        emit_callback_spans(line);
    }
}
```

## Performance Metrics

### Frame Statistics
```c
typedef struct {
    // Timing metrics
    uint64_t frame_count;
    uint64_t total_frame_time_ns;
    uint64_t min_frame_time_ns;
    uint64_t max_frame_time_ns;
    
    // Size metrics
    uint64_t total_spans;
    uint64_t merged_spans;
    uint64_t total_cells_updated;
    
    // Quality metrics
    uint64_t frames_dropped;
    uint64_t frames_coalesced;
    uint64_t perfect_frames;  // Delivered exactly on vsync
    
    // Histogram of frame times
    uint32_t frame_time_histogram[32];  // Buckets of 1ms each
} frame_stats_t;

void record_frame_metrics(frame_stats_t* stats, const frame_emit_info_t* info) {
    stats->frame_count++;
    stats->total_frame_time_ns += info->emit_time_ns;
    stats->min_frame_time_ns = MIN(stats->min_frame_time_ns, info->emit_time_ns);
    stats->max_frame_time_ns = MAX(stats->max_frame_time_ns, info->emit_time_ns);
    
    // Update histogram
    uint32_t bucket = info->emit_time_ns / 1000000;  // Convert to ms
    if (bucket < 32) {
        stats->frame_time_histogram[bucket]++;
    }
    
    // Check for perfect vsync alignment
    uint64_t vsync_distance = info->emit_time_ns % 16666667;  // 60 Hz
    if (vsync_distance < 1000000) {  // Within 1ms of vsync
        stats->perfect_frames++;
    }
}
```

## Testing and Validation

### Frame Batching Tests
```c
void test_frame_batching(void) {
    frame_aggregator_t agg = {
        .frame_interval_ns = 16666667,  // 60 FPS
        .max_latency_ns = 8000000,      // 8ms
    };
    
    // Test 1: Rapid updates should batch
    for (int i = 0; i < 100; i++) {
        tmc_grid_update_t update = make_test_update(i);
        aggregate_update(&agg, &update);
    }
    assert(agg.frames_emitted < 10);  // Should batch into <10 frames
    
    // Test 2: Urgent updates emit immediately
    tmc_grid_update_t urgent = make_cursor_update();
    urgent.flags = TMC_UPDATE_URGENT;
    uint64_t frames_before = agg.frames_emitted;
    aggregate_update(&agg, &urgent);
    assert(agg.frames_emitted == frames_before + 1);
    
    // Test 3: Vsync alignment
    reset_aggregator(&agg);
    simulate_vsync_updates(&agg, 1000);  // 1 second of updates
    assert(abs(agg.frames_emitted - 60) < 2);  // Should be ~60 frames
}
```

### Performance Benchmarks
```c
void benchmark_frame_aggregation(void) {
    const size_t iterations = 1000000;
    frame_aggregator_t agg = create_test_aggregator();
    
    uint64_t start = get_time_ns();
    
    for (size_t i = 0; i < iterations; i++) {
        tmc_grid_update_t update = generate_random_update();
        aggregate_update(&agg, &update);
    }
    
    uint64_t elapsed = get_time_ns() - start;
    double updates_per_sec = (double)iterations * 1e9 / elapsed;
    
    printf("Aggregation performance: %.2f updates/sec\n", updates_per_sec);
    printf("Frames emitted: %llu\n", agg.frames_emitted);
    printf("Merge ratio: %.2f%%\n", 
           100.0 * agg.spans_merged / agg.updates_batched);
    
    assert(updates_per_sec > 1000000);  // Should handle >1M updates/sec
}
```

## Summary

The frame batching system provides:
1. **Vsync-aligned delivery**: Frames emit at display refresh boundaries
2. **Intelligent aggregation**: Merges updates within frame budget
3. **Adaptive performance**: Adjusts frame rate based on load
4. **Efficient encoding**: Line-based updates when beneficial
5. **Robust recovery**: Handles drops and gaps gracefully
6. **Measurable quality**: Comprehensive metrics and statistics