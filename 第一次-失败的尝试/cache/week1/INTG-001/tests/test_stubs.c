// test_stubs.c - Stub implementations for testing
// Purpose: Provide mock implementations for external dependencies
// Author: INTG-001 (Zig-Ghostty Integration Specialist)
// Date: 2025-08-25

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../ARCH-001/ui_backend.h"
#include "../../ARCH-001/tty_ctx_unified.h"

// Ghostty FFI stubs
typedef struct ghostty_terminal {
    void* data;
} ghostty_terminal_t;

ghostty_terminal_t* ghostty_ffi_create_terminal(void* user_data) {
    ghostty_terminal_t* term = calloc(1, sizeof(ghostty_terminal_t));
    if (term) term->data = user_data;
    return term;
}

void ghostty_ffi_destroy_terminal(ghostty_terminal_t* terminal) {
    free(terminal);
}

uint32_t ghostty_ffi_get_capabilities(void) {
    return UI_CAP_FRAME_BATCH | UI_CAP_24BIT_COLOR | UI_CAP_UTF8_LINES;
}

void ghostty_ffi_register_callbacks(ghostty_terminal_t* terminal, void* callbacks) {
    (void)terminal;
    (void)callbacks;
    // No-op for testing
}

int ghostty_ffi_process_frame(ghostty_terminal_t* terminal, const ui_frame_t* frame) {
    (void)terminal;
    (void)frame;
    // Always succeed for testing
    return 0;
}

int ghostty_ffi_flush_immediate(ghostty_terminal_t* terminal) {
    (void)terminal;
    // Always succeed for testing
    return 0;
}

// Frame aggregator stubs - using the declared interface
frame_aggregator_t* frame_aggregator_create(uint32_t target_fps) {
    frame_aggregator_t* agg = calloc(1, sizeof(frame_aggregator_t));
    if (agg) {
        agg->size = sizeof(frame_aggregator_t);
        // Initialize other fields as needed
    }
    return agg;
}

void frame_aggregator_destroy(frame_aggregator_t* aggregator) {
    free(aggregator);
}

void frame_aggregator_add_update(frame_aggregator_t* agg,
                                 const struct tty_ctx* ctx) {
    (void)agg;
    (void)ctx;
    // No-op for testing
}

bool frame_aggregator_should_emit(const frame_aggregator_t* agg) {
    (void)agg;
    // Always emit in testing
    return true;
}

ui_frame_t* frame_aggregator_emit(frame_aggregator_t* aggregator) {
    (void)aggregator;
    // Return a minimal frame
    static ui_frame_t frame = {
        .size = sizeof(ui_frame_t),
        .frame_seq = 1,
        .timestamp_ns = 0,
        .pane_id = 1,
        .span_count = 0,
        .spans = NULL,
        .flags = UI_FRAME_COMPLETE
    };
    return &frame;
}

void frame_aggregator_reset(frame_aggregator_t* aggregator) {
    (void)aggregator;
    // No-op for testing
}

// UI cell conversion stub
void ui_cell_from_grid(ui_cell_t* ui_cell, const struct grid_cell* grid_cell) {
    (void)grid_cell;
    // Simple stub conversion
    ui_cell->codepoint = 'A';
    ui_cell->fg_rgb = 0xFFFFFF;
    ui_cell->bg_rgb = 0x000000;
    ui_cell->attrs = 0;
    ui_cell->width = 1;
    ui_cell->cluster_cont = 0;
}