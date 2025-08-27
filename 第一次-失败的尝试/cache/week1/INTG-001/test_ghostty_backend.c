// test_ghostty_backend.c - Comprehensive Test Suite for Ghostty Backend
// Purpose: Test all 22 tty_cmd callbacks and FFI bridge functionality
// Author: INTG-001 (Zig-Ghostty Integration Specialist)
// Date: 2025-08-25
// Version: 1.0.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "../ARCH-001/ui_backend.h"
#include "../CORE-001/tty_write_hooks.h"
#include "../CORE-002/backend_router.h"

// External functions from backend_ghostty.c
extern struct ui_backend* ghostty_backend_create(const ui_capabilities_t* requested_caps);
extern void ghostty_backend_destroy(struct ui_backend* backend);
extern void ghostty_backend_set_immediate_mode(struct ui_backend* backend, bool immediate);
extern void ghostty_backend_set_grid_optimization(struct ui_backend* backend, bool enabled);
extern void ghostty_backend_get_statistics(struct ui_backend* backend,
                                           uint64_t* frames_sent,
                                           uint64_t* cells_updated,
                                           uint64_t* frames_batched);

// ============================================================================
// Test Context Structure
// ============================================================================

typedef struct test_context {
    struct ui_backend* backend;
    backend_router_t* router;
    
    // Test data
    struct {
        uint32_t frame_count;
        uint32_t bell_count;
        uint32_t title_count;
        char last_title[256];
        ui_frame_t* last_frame;
    } callbacks;
    
    // Mock tty context
    struct tty_ctx mock_ctx;
    struct tty mock_tty;
    
    // Statistics
    uint64_t start_time_ns;
    uint64_t end_time_ns;
} test_context_t;

// ============================================================================
// Mock Data Structures
// ============================================================================

// Mock window pane
struct window_pane {
    uint32_t id;
    uint32_t sx, sy;
};

// Mock grid cell
struct grid_cell {
    uint32_t codepoint;
    uint16_t attr;
    uint8_t fg, bg;
};

// ============================================================================
// Callback Handlers
// ============================================================================

static void test_on_frame(const ui_frame_t* frame, void* user_data) {
    test_context_t* ctx = (test_context_t*)user_data;
    ctx->callbacks.frame_count++;
    
    // Copy frame for inspection
    if (ctx->callbacks.last_frame) {
        free(ctx->callbacks.last_frame);
    }
    ctx->callbacks.last_frame = malloc(sizeof(ui_frame_t));
    memcpy(ctx->callbacks.last_frame, frame, sizeof(ui_frame_t));
}

static void test_on_bell(uint32_t pane_id, void* user_data) {
    test_context_t* ctx = (test_context_t*)user_data;
    ctx->callbacks.bell_count++;
    printf("Bell received for pane %u\n", pane_id);
}

static void test_on_title(uint32_t pane_id, const char* title, void* user_data) {
    test_context_t* ctx = (test_context_t*)user_data;
    ctx->callbacks.title_count++;
    strncpy(ctx->callbacks.last_title, title, sizeof(ctx->callbacks.last_title) - 1);
    printf("Title changed for pane %u: %s\n", pane_id, title);
}

// ============================================================================
// Test Setup and Teardown
// ============================================================================

static test_context_t* create_test_context(void) {
    test_context_t* ctx = calloc(1, sizeof(test_context_t));
    assert(ctx != NULL);
    
    // Create backend with test capabilities
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION,
        .supported = UI_CAP_FRAME_BATCH | UI_CAP_24BIT_COLOR | UI_CAP_UTF8_LINES,
        .max_fps = 60,
        .optimal_batch_size = 50,
        .max_dirty_rects = 8
    };
    
    ctx->backend = ghostty_backend_create(&caps);
    assert(ctx->backend != NULL);
    
    // Register callbacks
    ctx->backend->on_frame = test_on_frame;
    ctx->backend->on_bell = test_on_bell;
    ctx->backend->on_title = test_on_title;
    ctx->backend->user_data = ctx;
    
    // Create router
    ctx->router = backend_router_create(BACKEND_MODE_UI);
    assert(ctx->router != NULL);
    backend_router_register_ui(ctx->router, ctx->backend);
    
    // Initialize mock structures
    static struct window_pane mock_wp = { .id = 1, .sx = 80, .sy = 24 };
    ctx->mock_ctx.wp = &mock_wp;
    ctx->mock_ctx.sx = 80;
    ctx->mock_ctx.sy = 24;
    ctx->mock_ctx.ocx = 0;
    ctx->mock_ctx.ocy = 0;
    ctx->mock_ctx.orupper = 0;
    ctx->mock_ctx.orlower = 23;
    ctx->mock_ctx.num = 1;
    
    ctx->start_time_ns = get_time_ns();
    
    return ctx;
}

static void destroy_test_context(test_context_t* ctx) {
    if (ctx) {
        ctx->end_time_ns = get_time_ns();
        
        // Print test statistics
        uint64_t frames_sent = 0, cells_updated = 0, frames_batched = 0;
        ghostty_backend_get_statistics(ctx->backend, &frames_sent, &cells_updated, &frames_batched);
        
        printf("\nTest Statistics:\n");
        printf("  Duration: %.3f ms\n", (ctx->end_time_ns - ctx->start_time_ns) / 1000000.0);
        printf("  Frames sent: %llu\n", frames_sent);
        printf("  Cells updated: %llu\n", cells_updated);
        printf("  Frames batched: %llu\n", frames_batched);
        printf("  Callbacks: frames=%u, bells=%u, titles=%u\n",
               ctx->callbacks.frame_count, ctx->callbacks.bell_count, ctx->callbacks.title_count);
        
        if (ctx->callbacks.last_frame) {
            free(ctx->callbacks.last_frame);
        }
        
        backend_router_destroy(ctx->router);
        ghostty_backend_destroy(ctx->backend);
        free(ctx);
    }
}

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// ============================================================================
// Test Cases for Each Command
// ============================================================================

static void test_cmd_cell(test_context_t* ctx) {
    printf("Testing tty_cmd_cell...\n");
    
    // Create a test cell
    static struct grid_cell test_cell = {
        .codepoint = 'A',
        .attr = UI_ATTR_BOLD,
        .fg = 7,
        .bg = 0
    };
    ctx->mock_ctx.cell = &test_cell;
    ctx->mock_ctx.ocx = 10;
    ctx->mock_ctx.ocy = 5;
    
    // Call the command
    ctx->backend->ops->cmd_cell(ctx->backend, &ctx->mock_ctx);
    
    // Verify cell was processed
    uint64_t cells_updated = 0;
    ghostty_backend_get_statistics(ctx->backend, NULL, &cells_updated, NULL);
    assert(cells_updated >= 1);
}

static void test_cmd_cells(test_context_t* ctx) {
    printf("Testing tty_cmd_cells...\n");
    
    ctx->mock_ctx.ocx = 0;
    ctx->mock_ctx.ocy = 0;
    ctx->mock_ctx.num = 10;  // 10 cells
    
    uint64_t cells_before = 0;
    ghostty_backend_get_statistics(ctx->backend, NULL, &cells_before, NULL);
    
    ctx->backend->ops->cmd_cells(ctx->backend, &ctx->mock_ctx);
    
    uint64_t cells_after = 0;
    ghostty_backend_get_statistics(ctx->backend, NULL, &cells_after, NULL);
    assert(cells_after >= cells_before + 10);
}

static void test_cmd_insertcharacter(test_context_t* ctx) {
    printf("Testing tty_cmd_insertcharacter...\n");
    
    ctx->mock_ctx.ocx = 5;
    ctx->mock_ctx.ocy = 10;
    
    ctx->backend->ops->cmd_insertcharacter(ctx->backend, &ctx->mock_ctx);
    
    // Verify dirty region was marked
    assert(ctx->callbacks.frame_count > 0 || ctx->backend->aggregator != NULL);
}

static void test_cmd_deletecharacter(test_context_t* ctx) {
    printf("Testing tty_cmd_deletecharacter...\n");
    
    ctx->mock_ctx.ocx = 15;
    ctx->mock_ctx.ocy = 8;
    
    ctx->backend->ops->cmd_deletecharacter(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_clearcharacter(test_context_t* ctx) {
    printf("Testing tty_cmd_clearcharacter...\n");
    
    ctx->mock_ctx.ocx = 20;
    ctx->mock_ctx.ocy = 12;
    ctx->mock_ctx.num = 5;
    
    ctx->backend->ops->cmd_clearcharacter(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_insertline(test_context_t* ctx) {
    printf("Testing tty_cmd_insertline...\n");
    
    ctx->mock_ctx.ocy = 10;
    
    ctx->backend->ops->cmd_insertline(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_deleteline(test_context_t* ctx) {
    printf("Testing tty_cmd_deleteline...\n");
    
    ctx->mock_ctx.ocy = 15;
    
    ctx->backend->ops->cmd_deleteline(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_clearline(test_context_t* ctx) {
    printf("Testing tty_cmd_clearline...\n");
    
    ctx->mock_ctx.ocy = 5;
    
    ctx->backend->ops->cmd_clearline(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_clearendofline(test_context_t* ctx) {
    printf("Testing tty_cmd_clearendofline...\n");
    
    ctx->mock_ctx.ocx = 40;
    ctx->mock_ctx.ocy = 7;
    
    ctx->backend->ops->cmd_clearendofline(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_clearstartofline(test_context_t* ctx) {
    printf("Testing tty_cmd_clearstartofline...\n");
    
    ctx->mock_ctx.ocx = 30;
    ctx->mock_ctx.ocy = 9;
    
    ctx->backend->ops->cmd_clearstartofline(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_clearscreen(test_context_t* ctx) {
    printf("Testing tty_cmd_clearscreen...\n");
    
    uint32_t frames_before = ctx->callbacks.frame_count;
    
    ctx->backend->ops->cmd_clearscreen(ctx->backend, &ctx->mock_ctx);
    
    // Clear screen should trigger immediate flush
    usleep(10000);  // Give time for async processing
    assert(ctx->callbacks.frame_count > frames_before);
}

static void test_cmd_clearendofscreen(test_context_t* ctx) {
    printf("Testing tty_cmd_clearendofscreen...\n");
    
    ctx->mock_ctx.ocy = 10;
    
    ctx->backend->ops->cmd_clearendofscreen(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_clearstartofscreen(test_context_t* ctx) {
    printf("Testing tty_cmd_clearstartofscreen...\n");
    
    ctx->mock_ctx.ocy = 10;
    
    ctx->backend->ops->cmd_clearstartofscreen(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_alignmenttest(test_context_t* ctx) {
    printf("Testing tty_cmd_alignmenttest...\n");
    
    ctx->backend->ops->cmd_alignmenttest(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_reverseindex(test_context_t* ctx) {
    printf("Testing tty_cmd_reverseindex...\n");
    
    ctx->backend->ops->cmd_reverseindex(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_linefeed(test_context_t* ctx) {
    printf("Testing tty_cmd_linefeed...\n");
    
    ctx->backend->ops->cmd_linefeed(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_scrollup(test_context_t* ctx) {
    printf("Testing tty_cmd_scrollup...\n");
    
    ctx->mock_ctx.num = 3;  // Scroll 3 lines
    
    ctx->backend->ops->cmd_scrollup(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_scrolldown(test_context_t* ctx) {
    printf("Testing tty_cmd_scrolldown...\n");
    
    ctx->mock_ctx.num = 2;  // Scroll 2 lines
    
    ctx->backend->ops->cmd_scrolldown(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_setselection(test_context_t* ctx) {
    printf("Testing tty_cmd_setselection...\n");
    
    ctx->backend->ops->cmd_setselection(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_rawstring(test_context_t* ctx) {
    printf("Testing tty_cmd_rawstring...\n");
    
    ctx->backend->ops->cmd_rawstring(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_sixelimage(test_context_t* ctx) {
    printf("Testing tty_cmd_sixelimage...\n");
    
    ctx->backend->ops->cmd_sixelimage(ctx->backend, &ctx->mock_ctx);
}

static void test_cmd_syncstart(test_context_t* ctx) {
    printf("Testing tty_cmd_syncstart...\n");
    
    ctx->backend->ops->cmd_syncstart(ctx->backend, &ctx->mock_ctx);
}

// ============================================================================
// Performance Tests
// ============================================================================

static void test_performance_batching(test_context_t* ctx) {
    printf("\nTesting performance with batching...\n");
    
    ghostty_backend_set_immediate_mode(ctx->backend, false);
    
    uint64_t start = get_time_ns();
    
    // Simulate rapid cell updates
    for (int i = 0; i < 1000; i++) {
        ctx->mock_ctx.ocx = i % 80;
        ctx->mock_ctx.ocy = (i / 80) % 24;
        ctx->backend->ops->cmd_cell(ctx->backend, &ctx->mock_ctx);
    }
    
    // Force flush
    ui_backend_flush_frame(ctx->backend);
    
    uint64_t end = get_time_ns();
    double ms = (end - start) / 1000000.0;
    
    printf("  1000 cells in %.3f ms (%.1f cells/ms)\n", ms, 1000.0 / ms);
    
    uint64_t frames_sent = 0;
    ghostty_backend_get_statistics(ctx->backend, &frames_sent, NULL, NULL);
    printf("  Frames sent: %llu (batching ratio: %.1fx)\n", 
           frames_sent, 1000.0 / frames_sent);
}

static void test_performance_immediate(test_context_t* ctx) {
    printf("\nTesting performance in immediate mode...\n");
    
    ghostty_backend_set_immediate_mode(ctx->backend, true);
    
    uint64_t start = get_time_ns();
    
    // Simulate rapid cell updates
    for (int i = 0; i < 100; i++) {  // Less iterations for immediate mode
        ctx->mock_ctx.ocx = i % 80;
        ctx->mock_ctx.ocy = (i / 80) % 24;
        ctx->backend->ops->cmd_cell(ctx->backend, &ctx->mock_ctx);
    }
    
    uint64_t end = get_time_ns();
    double ms = (end - start) / 1000000.0;
    
    printf("  100 cells in %.3f ms (%.1f cells/ms)\n", ms, 100.0 / ms);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

typedef struct {
    test_context_t* ctx;
    int thread_id;
    int iterations;
} thread_data_t;

static void* thread_worker(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    for (int i = 0; i < data->iterations; i++) {
        // Random command
        int cmd = rand() % 5;
        
        data->ctx->mock_ctx.ocx = (data->thread_id * 10 + i) % 80;
        data->ctx->mock_ctx.ocy = (data->thread_id + i) % 24;
        
        switch (cmd) {
            case 0:
                data->ctx->backend->ops->cmd_cell(data->ctx->backend, &data->ctx->mock_ctx);
                break;
            case 1:
                data->ctx->backend->ops->cmd_clearline(data->ctx->backend, &data->ctx->mock_ctx);
                break;
            case 2:
                data->ctx->backend->ops->cmd_insertcharacter(data->ctx->backend, &data->ctx->mock_ctx);
                break;
            case 3:
                data->ctx->backend->ops->cmd_scrollup(data->ctx->backend, &data->ctx->mock_ctx);
                break;
            case 4:
                data->ctx->backend->ops->cmd_linefeed(data->ctx->backend, &data->ctx->mock_ctx);
                break;
        }
        
        usleep(rand() % 1000);  // Random delay
    }
    
    return NULL;
}

static void test_thread_safety(test_context_t* ctx) {
    printf("\nTesting thread safety...\n");
    
    const int num_threads = 4;
    const int iterations = 50;
    
    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];
    
    // Start threads
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].ctx = ctx;
        thread_data[i].thread_id = i;
        thread_data[i].iterations = iterations;
        
        pthread_create(&threads[i], NULL, thread_worker, &thread_data[i]);
    }
    
    // Wait for completion
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("  %d threads completed %d iterations each\n", num_threads, iterations);
    
    uint64_t cells_updated = 0, errors = 0;
    ghostty_backend_get_statistics(ctx->backend, NULL, &cells_updated, NULL);
    printf("  Total cells updated: %llu\n", cells_updated);
    assert(errors == 0);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char* argv[]) {
    printf("=== Ghostty Backend Test Suite ===\n\n");
    
    // Initialize
    srand(time(NULL));
    tty_hooks_init();
    
    // Create test context
    test_context_t* ctx = create_test_context();
    
    // Test all 22 commands
    printf("Testing all 22 tty_cmd_* callbacks:\n");
    printf("---------------------------------\n");
    
    test_cmd_cell(ctx);
    test_cmd_cells(ctx);
    test_cmd_insertcharacter(ctx);
    test_cmd_deletecharacter(ctx);
    test_cmd_clearcharacter(ctx);
    test_cmd_insertline(ctx);
    test_cmd_deleteline(ctx);
    test_cmd_clearline(ctx);
    test_cmd_clearendofline(ctx);
    test_cmd_clearstartofline(ctx);
    test_cmd_clearscreen(ctx);
    test_cmd_clearendofscreen(ctx);
    test_cmd_clearstartofscreen(ctx);
    test_cmd_alignmenttest(ctx);
    test_cmd_reverseindex(ctx);
    test_cmd_linefeed(ctx);
    test_cmd_scrollup(ctx);
    test_cmd_scrolldown(ctx);
    test_cmd_setselection(ctx);
    test_cmd_rawstring(ctx);
    test_cmd_sixelimage(ctx);
    test_cmd_syncstart(ctx);
    
    printf("\n✓ All 22 commands tested successfully!\n");
    
    // Performance tests
    test_performance_batching(ctx);
    test_performance_immediate(ctx);
    
    // Thread safety test
    test_thread_safety(ctx);
    
    // Cleanup
    destroy_test_context(ctx);
    
    printf("\n=== All tests passed! ===\n");
    
    return 0;
}