// mock_backend.c - Mock UI Backend Implementation for Testing
// Author: QA-001 (Test Lead)
// Date: 2025-08-25
// Version: 1.0.0

#include "test_framework.h"
#include <assert.h>

// ============================================================================
// Mock Backend Callbacks
// ============================================================================

static void mock_on_frame(const ui_frame_t* frame, void* user_data) {
    mock_backend_t* mock = (mock_backend_t*)user_data;
    if (!mock || !mock->test_ctx) return;
    
    test_context_t* ctx = mock->test_ctx;
    
    // Validate frame
    if (mock->validate_frame_order && !validate_frame(frame)) {
        ctx->error_count++;
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Frame validation failed");
        return;
    }
    
    // Capture frame
    if (ctx->frame_count < TEST_MAX_FRAMES) {
        // Deep copy the frame
        size_t frame_size = sizeof(ui_frame_t);
        size_t spans_size = frame->span_count * sizeof(ui_span_t);
        
        ui_frame_t* captured = test_malloc(frame_size);
        if (captured) {
            memcpy(captured, frame, frame_size);
            
            // Copy spans
            if (frame->span_count > 0 && frame->spans) {
                ui_span_t* spans = test_malloc(spans_size);
                if (spans) {
                    memcpy(spans, frame->spans, spans_size);
                    captured->spans = spans;
                    
                    // Deep copy cells in each span
                    for (uint32_t i = 0; i < frame->span_count; i++) {
                        uint32_t cell_count = spans[i].col_end - spans[i].col_start;
                        size_t cells_size = cell_count * sizeof(ui_cell_t);
                        
                        ui_cell_t* cells = test_malloc(cells_size);
                        if (cells && frame->spans[i].cells) {
                            memcpy(cells, frame->spans[i].cells, cells_size);
                            spans[i].cells = cells;
                        }
                    }
                }
            }
            
            ctx->captured_frames[ctx->frame_count] = captured;
            ctx->frame_timestamps[ctx->frame_count] = get_time_ns();
            ctx->frame_count++;
        }
    }
    
    // Update statistics
    ctx->total_frames_emitted++;
    ctx->total_cells_updated += frame->cells_modified;
    ctx->total_spans_created += frame->span_count;
    
    if (frame->flags & UI_FRAME_DROPPED) {
        ctx->total_frames_dropped += frame->frames_dropped;
    }
    
    // Update latency tracking
    uint64_t now = get_time_ns();
    uint64_t latency = now - frame->timestamp_ns;
    
    if (latency < ctx->min_frame_latency_ns) {
        ctx->min_frame_latency_ns = latency;
    }
    if (latency > ctx->max_frame_latency_ns) {
        ctx->max_frame_latency_ns = latency;
    }
    
    // Validate span merging
    if (mock->validate_span_merging && !validate_span_merging(frame)) {
        ctx->error_count++;
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Span merging validation failed");
    }
    
    // Validate performance
    if (mock->validate_performance) {
        if (latency > TEST_MAX_LATENCY_MS * 1000000) {
            ctx->error_count++;
            snprintf(ctx->last_error, sizeof(ctx->last_error),
                    "Frame latency too high: %.2f ms", latency / 1000000.0);
        }
    }
}

static void mock_on_bell(uint32_t pane_id, void* user_data) {
    mock_backend_t* mock = (mock_backend_t*)user_data;
    if (!mock || !mock->test_ctx) return;
    
    // Just track that bell was triggered
    printf("BELL triggered for pane %u\n", pane_id);
}

static void mock_on_title(uint32_t pane_id, const char* title, void* user_data) {
    mock_backend_t* mock = (mock_backend_t*)user_data;
    if (!mock || !mock->test_ctx) return;
    
    printf("Title changed for pane %u: %s\n", pane_id, title);
}

static void mock_on_overflow(uint32_t dropped_frames, void* user_data) {
    mock_backend_t* mock = (mock_backend_t*)user_data;
    if (!mock || !mock->test_ctx) return;
    
    mock->test_ctx->total_frames_dropped += dropped_frames;
    printf("WARNING: %u frames dropped due to overflow\n", dropped_frames);
}

// ============================================================================
// Mock Backend Operations (22 tty_cmd_* functions)
// ============================================================================

static void mock_cmd_cell(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_cell_calls++;
    
    // Simulate processing
    if (mock->test_ctx) {
        mock->test_ctx->total_cells_updated++;
    }
}

static void mock_cmd_cells(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_cells_calls++;
    
    if (mock->test_ctx && ctx) {
        mock->test_ctx->total_cells_updated += ctx->num;
    }
}

static void mock_cmd_insertcharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_insertcharacter_calls++;
}

static void mock_cmd_deletecharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_deletecharacter_calls++;
}

static void mock_cmd_clearcharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_clearcharacter_calls++;
}

static void mock_cmd_insertline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_insertline_calls++;
}

static void mock_cmd_deleteline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_deleteline_calls++;
}

static void mock_cmd_clearline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_clearline_calls++;
    
    if (mock->test_ctx) {
        mock->test_ctx->total_cells_updated += 80; // Assume 80 column terminal
    }
}

static void mock_cmd_clearendofline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_clearendofline_calls++;
}

static void mock_cmd_clearstartofline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_clearstartofline_calls++;
}

static void mock_cmd_clearscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_clearscreen_calls++;
    
    if (mock->test_ctx) {
        mock->test_ctx->total_cells_updated += 80 * 24; // Standard terminal size
    }
}

static void mock_cmd_clearendofscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_clearendofscreen_calls++;
}

static void mock_cmd_clearstartofscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_clearstartofscreen_calls++;
}

static void mock_cmd_alignmenttest(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_alignmenttest_calls++;
    
    if (mock->test_ctx) {
        mock->test_ctx->total_cells_updated += 80 * 24;
    }
}

static void mock_cmd_reverseindex(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_reverseindex_calls++;
}

static void mock_cmd_linefeed(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_linefeed_calls++;
}

static void mock_cmd_scrollup(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_scrollup_calls++;
}

static void mock_cmd_scrolldown(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_scrolldown_calls++;
}

static void mock_cmd_setselection(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_setselection_calls++;
}

static void mock_cmd_rawstring(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_rawstring_calls++;
}

static void mock_cmd_sixelimage(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_sixelimage_calls++;
}

static void mock_cmd_syncstart(struct ui_backend* backend, const struct tty_ctx* ctx) {
    mock_backend_t* mock = (mock_backend_t*)backend;
    mock->cmd_syncstart_calls++;
}

// ============================================================================
// Mock Backend Operations Table
// ============================================================================

static ui_backend_ops_t mock_ops = {
    .size = sizeof(ui_backend_ops_t),
    .version = UI_BACKEND_ABI_VERSION,
    
    // Character/cell operations
    .cmd_cell = mock_cmd_cell,
    .cmd_cells = mock_cmd_cells,
    .cmd_insertcharacter = mock_cmd_insertcharacter,
    .cmd_deletecharacter = mock_cmd_deletecharacter,
    .cmd_clearcharacter = mock_cmd_clearcharacter,
    
    // Line operations
    .cmd_insertline = mock_cmd_insertline,
    .cmd_deleteline = mock_cmd_deleteline,
    .cmd_clearline = mock_cmd_clearline,
    .cmd_clearendofline = mock_cmd_clearendofline,
    .cmd_clearstartofline = mock_cmd_clearstartofline,
    
    // Screen operations
    .cmd_clearscreen = mock_cmd_clearscreen,
    .cmd_clearendofscreen = mock_cmd_clearendofscreen,
    .cmd_clearstartofscreen = mock_cmd_clearstartofscreen,
    .cmd_alignmenttest = mock_cmd_alignmenttest,
    
    // Scrolling operations
    .cmd_reverseindex = mock_cmd_reverseindex,
    .cmd_linefeed = mock_cmd_linefeed,
    .cmd_scrollup = mock_cmd_scrollup,
    .cmd_scrolldown = mock_cmd_scrolldown,
    
    // Special operations
    .cmd_setselection = mock_cmd_setselection,
    .cmd_rawstring = mock_cmd_rawstring,
    .cmd_sixelimage = mock_cmd_sixelimage,
    .cmd_syncstart = mock_cmd_syncstart,
};

// ============================================================================
// Mock Backend Creation and Management
// ============================================================================

mock_backend_t* mock_backend_create(test_context_t* ctx) {
    mock_backend_t* mock = test_malloc(sizeof(mock_backend_t));
    if (!mock) return NULL;
    
    memset(mock, 0, sizeof(mock_backend_t));
    
    // Initialize base backend
    mock->base.size = sizeof(struct ui_backend);
    mock->base.version = UI_BACKEND_ABI_VERSION;
    mock->base.type = UI_BACKEND_TEST;
    mock->base.ops = &mock_ops;
    
    // Set up capabilities
    mock->base.capabilities.size = sizeof(ui_capabilities_t);
    mock->base.capabilities.version = UI_BACKEND_ABI_VERSION;
    mock->base.capabilities.supported = UI_CAP_FRAME_BATCH |
                                       UI_CAP_24BIT_COLOR |
                                       UI_CAP_SYNCHRONIZED;
    mock->base.capabilities.max_fps = 60;
    mock->base.capabilities.optimal_batch_size = 100;
    mock->base.capabilities.max_dirty_rects = 10;
    
    // Set up callbacks
    mock->base.on_frame = mock_on_frame;
    mock->base.on_bell = mock_on_bell;
    mock->base.on_title = mock_on_title;
    mock->base.on_overflow = mock_on_overflow;
    mock->base.user_data = mock;
    
    // Link to test context
    mock->test_ctx = ctx;
    
    // Enable default validations
    mock->validate_frame_order = true;
    mock->validate_span_merging = true;
    mock->validate_memory_usage = true;
    mock->validate_performance = true;
    
    return mock;
}

void mock_backend_destroy(mock_backend_t* backend) {
    if (!backend) return;
    
    // Clean up any allocated resources
    if (backend->base.aggregator) {
        test_free(backend->base.aggregator);
    }
    
    test_free(backend);
}

void mock_backend_reset_counters(mock_backend_t* backend) {
    if (!backend) return;
    
    backend->cmd_cell_calls = 0;
    backend->cmd_cells_calls = 0;
    backend->cmd_clearline_calls = 0;
    backend->cmd_clearscreen_calls = 0;
    backend->cmd_scrollup_calls = 0;
    backend->cmd_scrolldown_calls = 0;
    backend->cmd_insertline_calls = 0;
    backend->cmd_deleteline_calls = 0;
    backend->cmd_insertcharacter_calls = 0;
    backend->cmd_deletecharacter_calls = 0;
    backend->cmd_clearcharacter_calls = 0;
    backend->cmd_clearendofline_calls = 0;
    backend->cmd_clearstartofline_calls = 0;
    backend->cmd_clearendofscreen_calls = 0;
    backend->cmd_clearstartofscreen_calls = 0;
    backend->cmd_reverseindex_calls = 0;
    backend->cmd_linefeed_calls = 0;
    backend->cmd_alignmenttest_calls = 0;
    backend->cmd_setselection_calls = 0;
    backend->cmd_rawstring_calls = 0;
    backend->cmd_sixelimage_calls = 0;
    backend->cmd_syncstart_calls = 0;
}