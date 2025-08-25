// backend_ghostty_example.c - Complete Example Implementation for INTG-001
// Author: ARCH-001 (System Architect)
// Purpose: Example implementation showing how to use the fixed interfaces
// Version: 2.0.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../tty_ctx_unified.h"
#include "../ui_backend_callbacks_fixed.h"
#include "../interface_adapter.h"

// ============================================================================
// Example: Complete Ghostty Backend Implementation
// ============================================================================

// Private backend data
typedef struct {
    ui_backend_v2_t base;
    void* grid_buffer;
    size_t grid_size;
    int connection_fd;
} ghostty_backend_t;

// ============================================================================
// Implementation of All 22 Callbacks
// ============================================================================

static void ghostty_cmd_cell_impl(struct ui_backend* backend, const struct tty_ctx* ctx) {
    // Validate context
    if (!tty_ctx_is_valid(ctx)) {
        fprintf(stderr, "[Ghostty] Invalid tty_ctx in cmd_cell\n");
        return;
    }
    
    // Safe field access
    uint32_t row = TTY_CTX_GET_FIELD(ctx, ocy, 0);
    uint32_t col = TTY_CTX_GET_FIELD(ctx, ocx, 0);
    ui_cell_t* cell = (ui_cell_t*)TTY_CTX_GET_FIELD(ctx, cell, NULL);
    
    if (!cell) {
        fprintf(stderr, "[Ghostty] No cell data provided\n");
        return;
    }
    
    // Process the cell
    printf("[Ghostty] Writing cell at (%u, %u): char='%c' fg=%08x bg=%08x\n",
           row, col, cell->data[0], cell->attr.fg, cell->attr.bg);
    
    // Update statistics
    ((ui_backend_v2_t*)backend)->stats.commands_processed++;
    ((ui_backend_v2_t*)backend)->stats.callbacks_invoked++;
}

static void ghostty_cmd_cells_impl(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (!tty_ctx_is_valid(ctx)) return;
    
    uint32_t row = TTY_CTX_GET_FIELD(ctx, ocy, 0);
    uint32_t col = TTY_CTX_GET_FIELD(ctx, ocx, 0);
    uint32_t count = TTY_CTX_GET_FIELD(ctx, num, 0);
    ui_cell_t* cells = (ui_cell_t*)TTY_CTX_GET_FIELD(ctx, ptr, NULL);
    
    printf("[Ghostty] Writing %u cells starting at (%u, %u)\n", count, row, col);
    
    // Process batch of cells
    for (uint32_t i = 0; i < count && cells; i++) {
        // Process each cell
        // In real implementation, update the grid buffer
    }
    
    ((ui_backend_v2_t*)backend)->stats.commands_processed++;
}

static void ghostty_cmd_clearscreen_impl(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (!tty_ctx_is_valid(ctx)) return;
    
    printf("[Ghostty] Clearing entire screen\n");
    
    // This is an urgent operation - emit frame immediately
    ui_backend_v2_t* backend_v2 = (ui_backend_v2_t*)backend;
    if (backend_v2->on_frame) {
        ui_frame_t frame = {
            .type = FRAME_TYPE_CLEAR,
            .urgent = 1,
            .timestamp = 0, // Would use actual timestamp
            .spans = NULL,
            .span_count = 0
        };
        backend_v2->on_frame(backend_v2->callback_context, &frame);
    }
    
    backend_v2->stats.commands_processed++;
    backend_v2->stats.frames_emitted++;
}

// ... Implement remaining 19 callbacks similarly ...

// ============================================================================
// Backend Creation and Initialization
// ============================================================================

ghostty_backend_t* create_ghostty_backend_full(void) {
    // Allocate backend structure
    ghostty_backend_t* backend = calloc(1, sizeof(ghostty_backend_t));
    if (!backend) return NULL;
    
    // Initialize base structure
    backend->base.size = sizeof(ui_backend_v2_t);
    backend->base.version = 2;
    backend->base.type = UI_BACKEND_GHOSTTY;
    
    // Create operations table
    ui_backend_ops_v2_t* ops = calloc(1, sizeof(ui_backend_ops_v2_t));
    if (!ops) {
        free(backend);
        return NULL;
    }
    
    // Initialize operations table
    ops->size = sizeof(ui_backend_ops_v2_t);
    ops->version = 2;
    
    // Assign all 22 callbacks
    ops->cmd_cell = ghostty_cmd_cell_impl;
    ops->cmd_cells = ghostty_cmd_cells_impl;
    ops->cmd_insertcharacter = ghostty_cmd_cell_impl; // Simplified for example
    ops->cmd_deletecharacter = ghostty_cmd_cell_impl;
    ops->cmd_clearcharacter = ghostty_cmd_cell_impl;
    ops->cmd_insertline = ghostty_cmd_cell_impl;
    ops->cmd_deleteline = ghostty_cmd_cell_impl;
    ops->cmd_clearline = ghostty_cmd_cell_impl;
    ops->cmd_clearendofline = ghostty_cmd_cell_impl;
    ops->cmd_clearstartofline = ghostty_cmd_cell_impl;
    ops->cmd_clearscreen = ghostty_cmd_clearscreen_impl;
    ops->cmd_clearendofscreen = ghostty_cmd_clearscreen_impl;
    ops->cmd_clearstartofscreen = ghostty_cmd_clearscreen_impl;
    ops->cmd_alignmenttest = ghostty_cmd_cell_impl;
    ops->cmd_reverseindex = ghostty_cmd_cell_impl;
    ops->cmd_linefeed = ghostty_cmd_cell_impl;
    ops->cmd_scrollup = ghostty_cmd_cell_impl;
    ops->cmd_scrolldown = ghostty_cmd_cell_impl;
    ops->cmd_setselection = ghostty_cmd_cell_impl;
    ops->cmd_rawstring = ghostty_cmd_cell_impl;
    ops->cmd_sixelimage = ghostty_cmd_cell_impl;
    ops->cmd_syncstart = ghostty_cmd_cell_impl;
    
    backend->base.ops = ops;
    
    // Set capabilities
    backend->base.capabilities.supported = 
        UI_CAP_FRAME_BATCH | 
        UI_CAP_24BIT_COLOR |
        UI_CAP_SYNCHRONIZED;
    backend->base.capabilities.max_fps = 60;
    backend->base.capabilities.optimal_batch_size = 100;
    
    // Initialize private data
    backend->grid_size = 80 * 24 * sizeof(ui_cell_t);
    backend->grid_buffer = calloc(1, backend->grid_size);
    backend->connection_fd = -1;
    
    printf("[Ghostty] Backend created successfully with all 22 callbacks\n");
    return backend;
}

// ============================================================================
// Integration Example
// ============================================================================

int main(void) {
    printf("=== Ghostty Backend Integration Example ===\n\n");
    
    // Step 1: Initialize interfaces
    printf("1. Initializing interfaces...\n");
    int result = tty_hooks_init();
    if (result != 0) {
        fprintf(stderr, "Failed to initialize hooks\n");
        return 1;
    }
    
    // Step 2: Create backend
    printf("2. Creating Ghostty backend...\n");
    ghostty_backend_t* backend = create_ghostty_backend_full();
    if (!backend) {
        fprintf(stderr, "Failed to create backend\n");
        return 1;
    }
    
    // Step 3: Create and register with router
    printf("3. Creating router and registering backend...\n");
    struct backend_router* router = NULL;
    result = backend_router_create(&router);
    if (result != 0) {
        fprintf(stderr, "Failed to create router\n");
        return 1;
    }
    
    result = backend_router_register(router, (struct ui_backend*)backend);
    if (result != 0) {
        fprintf(stderr, "Failed to register backend\n");
        return 1;
    }
    
    // Step 4: Test with sample commands
    printf("\n4. Testing with sample commands...\n");
    
    // Create a test context
    struct tty_ctx ctx = {0};
    tty_ctx_init(&ctx);
    ctx.ocx = 10;
    ctx.ocy = 5;
    
    // Create a test cell
    ui_cell_t test_cell = {
        .data = {'A', 0, 0, 0},
        .attr = {
            .fg = 0x00FF00, // Green
            .bg = 0x000000, // Black
            .flags = 0
        }
    };
    ctx.cell = &test_cell;
    
    // Test cmd_cell
    printf("\nTesting cmd_cell:\n");
    backend->base.ops->cmd_cell((struct ui_backend*)backend, &ctx);
    
    // Test cmd_clearscreen
    printf("\nTesting cmd_clearscreen:\n");
    backend->base.ops->cmd_clearscreen((struct ui_backend*)backend, &ctx);
    
    // Step 5: Check statistics
    printf("\n5. Backend Statistics:\n");
    printf("   Commands processed: %lu\n", backend->base.stats.commands_processed);
    printf("   Frames emitted: %lu\n", backend->base.stats.frames_emitted);
    printf("   Callbacks invoked: %lu\n", backend->base.stats.callbacks_invoked);
    
    // Step 6: Test compatibility layer
    printf("\n6. Testing compatibility layer...\n");
    result = tty_write_hooks_init(); // Should work but print warning
    
    // Cleanup
    printf("\n7. Cleanup...\n");
    backend_router_destroy(router);
    free(backend->grid_buffer);
    free((void*)backend->base.ops);
    free(backend);
    tty_hooks_cleanup();
    
    printf("\n✅ Integration example completed successfully!\n");
    return 0;
}