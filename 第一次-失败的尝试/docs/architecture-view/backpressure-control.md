# Backpressure Control and Flow Management

## Overview
This document defines the backpressure mechanisms that prevent tmux from overwhelming Ghostty's rendering pipeline during high-throughput scenarios (e.g., `yes | pv`, large compilation outputs, log tailing).

## Core Concepts

### Ring Buffer Architecture
```c
typedef struct {
    // Configuration
    uint32_t capacity;          // Maximum frames to buffer
    uint32_t watermark_low;     // Resume threshold (e.g., 50%)
    uint32_t watermark_high;    // Pause threshold (e.g., 80%)
    uint32_t watermark_drop;    // Drop threshold (e.g., 90%)
    
    // Ring buffer state
    tmc_frame_t* frames;        // Circular buffer
    uint32_t head;             // Write position
    uint32_t tail;             // Read position
    uint32_t count;            // Current occupancy
    
    // Flow control state
    tmc_flow_state_t state;    // FLOWING, PAUSED, DROPPING
    uint64_t state_changes;    // State transition count
    
    // Statistics
    uint64_t frames_written;
    uint64_t frames_read;
    uint64_t frames_dropped;
    uint64_t pause_duration_ns;
} frame_ring_buffer_t;

typedef enum {
    TMC_FLOW_FLOWING,   // Normal operation
    TMC_FLOW_PAUSED,    // Backpressure applied
    TMC_FLOW_DROPPING,  // Dropping frames to catch up
    TMC_FLOW_RECOVERING // Draining buffer after drop
} tmc_flow_state_t;
```

## Backpressure Strategy

### Multi-Level Thresholds
```c
typedef struct {
    // Percentage thresholds (0-100)
    uint8_t green_zone;    // 0-50%: Full speed
    uint8_t yellow_zone;   // 50-80%: Start throttling
    uint8_t red_zone;      // 80-90%: Heavy throttling
    uint8_t drop_zone;     // 90-100%: Drop frames
    
    // Throttling parameters
    uint32_t yellow_delay_us;  // Microseconds to delay in yellow
    uint32_t red_delay_us;     // Microseconds to delay in red
    
    // Recovery parameters
    uint32_t recovery_threshold;  // Resume at this percentage
    uint32_t burst_allowance;     // Frames allowed in burst
} backpressure_config_t;

// Default configuration
static const backpressure_config_t default_config = {
    .green_zone = 50,
    .yellow_zone = 80,
    .red_zone = 90,
    .drop_zone = 95,
    .yellow_delay_us = 100,
    .red_delay_us = 1000,
    .recovery_threshold = 40,
    .burst_allowance = 10
};
```

### Flow Control Algorithm
```c
tmc_flow_action_t evaluate_backpressure(frame_ring_buffer_t* buffer) {
    uint32_t occupancy_pct = (buffer->count * 100) / buffer->capacity;
    tmc_flow_action_t action = {0};
    
    // State machine for flow control
    switch (buffer->state) {
    case TMC_FLOW_FLOWING:
        if (occupancy_pct >= buffer->watermark_drop) {
            // Emergency: start dropping
            buffer->state = TMC_FLOW_DROPPING;
            action.drop = true;
            send_overflow_notification(buffer);
        } else if (occupancy_pct >= buffer->watermark_high) {
            // High water mark: pause input
            buffer->state = TMC_FLOW_PAUSED;
            action.pause = true;
            buffer->pause_start = get_time_ns();
        } else if (occupancy_pct >= 50) {
            // Mild pressure: throttle
            action.delay_us = calculate_throttle_delay(occupancy_pct);
        }
        break;
        
    case TMC_FLOW_PAUSED:
        if (occupancy_pct <= buffer->watermark_low) {
            // Low water mark: resume
            buffer->state = TMC_FLOW_FLOWING;
            action.resume = true;
            buffer->pause_duration_ns += get_time_ns() - buffer->pause_start;
        } else if (occupancy_pct >= buffer->watermark_drop) {
            // Still climbing despite pause: drop
            buffer->state = TMC_FLOW_DROPPING;
            action.drop = true;
        }
        break;
        
    case TMC_FLOW_DROPPING:
        // Drop all non-essential frames
        action.drop = !is_essential_frame(buffer->frames[buffer->head]);
        
        if (occupancy_pct <= buffer->watermark_low) {
            // Recovered: enter recovery state
            buffer->state = TMC_FLOW_RECOVERING;
            action.drop = false;
            send_recovery_notification(buffer);
        }
        break;
        
    case TMC_FLOW_RECOVERING:
        // Gradual recovery to avoid oscillation
        if (occupancy_pct <= 25) {
            buffer->state = TMC_FLOW_FLOWING;
        } else {
            // Mild throttling during recovery
            action.delay_us = 50;
        }
        break;
    }
    
    action.occupancy_pct = occupancy_pct;
    return action;
}
```

## Per-Pane Flow Control

### Pane-Specific Buffers
```c
typedef struct {
    uint32_t pane_id;
    frame_ring_buffer_t buffer;
    
    // Pane-specific metrics
    uint64_t bytes_written;
    uint64_t bytes_rendered;
    double throughput_mbps;
    
    // Priority and QoS
    uint8_t priority;      // 0-255, higher = more important
    uint8_t qos_class;     // Interactive, bulk, background
    
    // Rate limiting
    token_bucket_t rate_limiter;
} pane_buffer_t;

typedef struct {
    pane_buffer_t* panes;
    uint32_t pane_count;
    uint32_t total_capacity;  // Shared memory pool
    
    // Global pressure
    uint32_t global_pressure_pct;
    
    // Fair queuing
    weighted_fair_queue_t* scheduler;
} multi_pane_controller_t;
```

### Weighted Fair Queuing
```c
// Distribute rendering capacity fairly among panes
void schedule_pane_frames(multi_pane_controller_t* controller) {
    // Calculate each pane's share based on priority
    uint32_t total_weight = 0;
    for (uint32_t i = 0; i < controller->pane_count; i++) {
        total_weight += controller->panes[i].priority;
    }
    
    // Allocate frame budget
    uint32_t frame_budget = 60;  // Target FPS
    
    for (uint32_t i = 0; i < controller->pane_count; i++) {
        pane_buffer_t* pane = &controller->panes[i];
        uint32_t pane_budget = (frame_budget * pane->priority) / total_weight;
        
        // Process up to budget frames
        uint32_t processed = 0;
        while (processed < pane_budget && has_frames(&pane->buffer)) {
            process_frame(pane);
            processed++;
        }
        
        // Apply backpressure if over budget
        if (has_frames(&pane->buffer)) {
            apply_pane_backpressure(pane);
        }
    }
}
```

## Smart Frame Dropping

### Frame Priority Classification
```c
typedef enum {
    TMC_FRAME_PRIORITY_CRITICAL,   // Cursor, bell, user input echo
    TMC_FRAME_PRIORITY_HIGH,       // Partial updates, scrolling
    TMC_FRAME_PRIORITY_NORMAL,     // Regular content updates  
    TMC_FRAME_PRIORITY_LOW,        // Background updates
    TMC_FRAME_PRIORITY_BULK,       // Large data dumps
} tmc_frame_priority_t;

tmc_frame_priority_t classify_frame(const tmc_frame_t* frame) {
    if (frame->flags & TMC_FRAME_CURSOR) {
        return TMC_FRAME_PRIORITY_CRITICAL;
    }
    if (frame->flags & TMC_FRAME_USER_INPUT) {
        return TMC_FRAME_PRIORITY_CRITICAL;
    }
    if (frame->span_count == 1 && frame->spans[0].cell_count < 10) {
        return TMC_FRAME_PRIORITY_HIGH;  // Small update
    }
    if (frame->span_count > 100) {
        return TMC_FRAME_PRIORITY_BULK;  // Large update
    }
    return TMC_FRAME_PRIORITY_NORMAL;
}
```

### Intelligent Frame Coalescing
```c
// When dropping, coalesce multiple frames into one
typedef struct {
    tmc_frame_t* coalesced_frame;
    uint32_t source_frame_count;
    uint64_t time_span_ns;
    
    // Dirty region tracking
    dirty_rect_t dirty_regions[MAX_REGIONS];
    uint32_t region_count;
} frame_coalescer_t;

void coalesce_frames(frame_coalescer_t* coalescer, 
                    tmc_frame_t* frames[], 
                    uint32_t count) {
    // Start with empty dirty region
    reset_dirty_regions(coalescer);
    
    for (uint32_t i = 0; i < count; i++) {
        tmc_frame_t* frame = frames[i];
        
        // Skip low priority frames entirely
        if (classify_frame(frame) == TMC_FRAME_PRIORITY_BULK) {
            continue;
        }
        
        // Merge dirty regions
        for (uint32_t j = 0; j < frame->span_count; j++) {
            add_dirty_span(coalescer, &frame->spans[j]);
        }
    }
    
    // Generate coalesced frame from dirty regions
    coalescer->coalesced_frame = generate_frame_from_regions(coalescer);
    coalescer->source_frame_count = count;
}
```

## Token Bucket Rate Limiting

### Rate Limiter Implementation
```c
typedef struct {
    // Configuration
    uint64_t rate_bps;         // Bytes per second
    uint64_t burst_size;       // Maximum burst in bytes
    
    // State
    uint64_t tokens;           // Available tokens (bytes)
    uint64_t last_refill_ns;   // Last refill timestamp
    
    // Statistics
    uint64_t bytes_allowed;
    uint64_t bytes_dropped;
    uint64_t wait_time_total_ns;
} token_bucket_t;

bool token_bucket_consume(token_bucket_t* bucket, uint64_t bytes) {
    // Refill tokens based on elapsed time
    uint64_t now = get_time_ns();
    uint64_t elapsed = now - bucket->last_refill_ns;
    uint64_t new_tokens = (bucket->rate_bps * elapsed) / 1000000000;
    
    bucket->tokens = MIN(bucket->tokens + new_tokens, bucket->burst_size);
    bucket->last_refill_ns = now;
    
    // Try to consume tokens
    if (bucket->tokens >= bytes) {
        bucket->tokens -= bytes;
        bucket->bytes_allowed += bytes;
        return true;
    }
    
    // Not enough tokens, calculate wait time
    uint64_t deficit = bytes - bucket->tokens;
    uint64_t wait_ns = (deficit * 1000000000) / bucket->rate_bps;
    
    bucket->bytes_dropped += bytes;
    bucket->wait_time_total_ns += wait_ns;
    
    return false;
}
```

## Overflow Notifications

### Notification System
```c
typedef enum {
    TMC_OVERFLOW_WARNING,      // Approaching limits
    TMC_OVERFLOW_CRITICAL,     // Dropping frames
    TMC_OVERFLOW_RECOVERED,    // Back to normal
} tmc_overflow_level_t;

typedef struct {
    tmc_overflow_level_t level;
    uint32_t pane_id;
    uint64_t timestamp_ns;
    
    // Metrics at time of notification
    uint32_t buffer_occupancy_pct;
    uint64_t frames_dropped;
    uint64_t bytes_dropped;
    double input_rate_mbps;
    double render_rate_mbps;
    
    // Suggested action
    const char* suggestion;
} tmc_overflow_notification_t;

// Callback for overflow conditions
typedef void (*tmc_overflow_callback_t)(
    tmc_client_t* client,
    const tmc_overflow_notification_t* notification
);

void send_overflow_notification(frame_ring_buffer_t* buffer, 
                               tmc_overflow_level_t level) {
    tmc_overflow_notification_t notif = {
        .level = level,
        .pane_id = buffer->pane_id,
        .timestamp_ns = get_time_ns(),
        .buffer_occupancy_pct = (buffer->count * 100) / buffer->capacity,
        .frames_dropped = buffer->frames_dropped,
        .input_rate_mbps = calculate_input_rate(buffer),
        .render_rate_mbps = calculate_render_rate(buffer)
    };
    
    // Add suggestions based on condition
    switch (level) {
    case TMC_OVERFLOW_WARNING:
        notif.suggestion = "Consider slowing input or increasing buffer size";
        break;
    case TMC_OVERFLOW_CRITICAL:
        notif.suggestion = "Frames being dropped, reduce input rate";
        break;
    case TMC_OVERFLOW_RECOVERED:
        notif.suggestion = "Normal operation resumed";
        break;
    }
    
    // Invoke callback
    if (buffer->overflow_callback) {
        buffer->overflow_callback(buffer->client, &notif);
    }
}
```

## Adaptive Buffer Sizing

### Dynamic Buffer Management
```c
typedef struct {
    uint32_t min_size;         // Minimum buffer size
    uint32_t max_size;         // Maximum buffer size
    uint32_t current_size;     // Current allocation
    
    // Adaptation parameters
    float growth_factor;       // Multiplicative increase (e.g., 1.5)
    float shrink_factor;       // Multiplicative decrease (e.g., 0.9)
    uint64_t adapt_interval_ns; // How often to adjust
    
    // History for decision making
    uint32_t overflow_count;
    uint32_t underflow_count;
    uint64_t last_adapt_time_ns;
} adaptive_buffer_t;

void adapt_buffer_size(adaptive_buffer_t* buffer, const buffer_stats_t* stats) {
    uint64_t now = get_time_ns();
    if (now - buffer->last_adapt_time_ns < buffer->adapt_interval_ns) {
        return;  // Too soon to adapt
    }
    
    // Calculate average occupancy
    float avg_occupancy = stats->avg_occupancy_pct;
    
    if (stats->overflow_events > 0) {
        // Had overflows, increase size
        uint32_t new_size = buffer->current_size * buffer->growth_factor;
        buffer->current_size = MIN(new_size, buffer->max_size);
        buffer->overflow_count = 0;
    } else if (avg_occupancy < 25 && buffer->current_size > buffer->min_size) {
        // Underutilized, shrink to save memory
        uint32_t new_size = buffer->current_size * buffer->shrink_factor;
        buffer->current_size = MAX(new_size, buffer->min_size);
    }
    
    buffer->last_adapt_time_ns = now;
}
```

## Performance Monitoring

### Backpressure Metrics
```c
typedef struct {
    // Flow control events
    uint64_t pause_events;
    uint64_t resume_events;
    uint64_t drop_events;
    
    // Time spent in states
    uint64_t time_flowing_ns;
    uint64_t time_paused_ns;
    uint64_t time_dropping_ns;
    
    // Frame statistics
    uint64_t frames_processed;
    uint64_t frames_dropped;
    uint64_t frames_coalesced;
    
    // Throughput metrics
    double avg_input_rate_mbps;
    double avg_output_rate_mbps;
    double peak_input_rate_mbps;
    
    // Latency metrics
    uint64_t min_latency_ns;
    uint64_t max_latency_ns;
    uint64_t avg_latency_ns;
    
    // Buffer utilization
    uint32_t min_occupancy_pct;
    uint32_t max_occupancy_pct;
    uint32_t avg_occupancy_pct;
} backpressure_stats_t;

void export_backpressure_metrics(const backpressure_stats_t* stats, 
                                 tmc_metrics_t* metrics) {
    // Export to Prometheus format
    metric_gauge(metrics, "tmux_buffer_occupancy_percent", 
                stats->avg_occupancy_pct);
    metric_counter(metrics, "tmux_frames_dropped_total", 
                  stats->frames_dropped);
    metric_histogram(metrics, "tmux_frame_latency_seconds", 
                    stats->avg_latency_ns / 1e9);
    metric_gauge(metrics, "tmux_input_rate_mbps", 
                stats->avg_input_rate_mbps);
}
```

## Testing Strategies

### Stress Testing
```c
void test_backpressure_handling(void) {
    frame_ring_buffer_t buffer = {
        .capacity = 256,
        .watermark_high = 204,  // 80%
        .watermark_drop = 230,  // 90%
        .watermark_low = 128,   // 50%
    };
    
    // Test 1: Burst handling
    for (int i = 0; i < 1000; i++) {
        tmc_frame_t frame = generate_test_frame(i);
        tmc_flow_action_t action = evaluate_backpressure(&buffer);
        
        if (!action.drop) {
            ring_buffer_write(&buffer, &frame);
        }
        
        // Simulate slow consumer
        if (i % 3 == 0) {
            ring_buffer_read(&buffer);
        }
    }
    
    assert(buffer.frames_dropped > 0);  // Should have dropped some
    assert(buffer.state_changes > 2);   // Should have transitioned states
    
    // Test 2: Recovery behavior
    while (buffer.count > 0) {
        ring_buffer_read(&buffer);
    }
    assert(buffer.state == TMC_FLOW_FLOWING);
    
    // Test 3: Steady state
    simulate_steady_traffic(&buffer, 10000);
    assert(buffer.frames_dropped == 0);  // Should handle steady load
}
```

### Benchmarks
```c
void benchmark_backpressure_overhead(void) {
    const size_t iterations = 10000000;
    frame_ring_buffer_t buffer = create_test_buffer(1024);
    
    uint64_t start = get_time_ns();
    
    for (size_t i = 0; i < iterations; i++) {
        tmc_flow_action_t action = evaluate_backpressure(&buffer);
        // Simulate work based on action
        if (action.delay_us > 0) {
            // Would delay in real scenario
        }
    }
    
    uint64_t elapsed = get_time_ns() - start;
    double checks_per_sec = (double)iterations * 1e9 / elapsed;
    
    printf("Backpressure evaluation: %.2f checks/sec\n", checks_per_sec);
    printf("Overhead per check: %.2f ns\n", elapsed / (double)iterations);
    
    assert(checks_per_sec > 10000000);  // Should handle >10M checks/sec
}
```

## Summary

The backpressure control system provides:
1. **Multi-level flow control**: Progressive throttling before dropping
2. **Per-pane isolation**: Independent buffers prevent interference
3. **Smart frame dropping**: Priority-based with coalescing
4. **Adaptive sizing**: Buffers adjust to workload
5. **Observable behavior**: Rich metrics and notifications
6. **Predictable performance**: Bounded latency and memory usage