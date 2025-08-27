// integration_example.c - Complete Integration Example
// Purpose: Demonstrate full integration of Ghostty backend with tmux
// Author: INTG-001 (Zig-Ghostty Integration Specialist)
// Date: 2025-08-25
// Version: 1.0.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "backend_ghostty.h"
#include "../ARCH-001/backend_router.h"
#include "../CORE-001/tty_write_hooks.h"

// Global state
static struct {
    struct ui_backend* ghostty_backend;
    backend_router_t* router;
    volatile sig_atomic_t running;
} g_state = { NULL, NULL, 1 };

// ============================================================================
// Signal Handling
// ============================================================================

static void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    g_state.running = 0;
}

// ============================================================================
// Callback Handlers
// ============================================================================

static void on_frame_received(const ui_frame_t* frame, void* user_data) {
    static uint64_t frame_count = 0;
    frame_count++;
    
    if (frame_count % 100 == 0) {
        printf("Frame %llu: %u spans, %u cells modified\n",
               frame_count, frame->span_count, frame->cells_modified);
    }
}

static void on_bell(uint32_t pane_id, void* user_data) {
    printf("\a");  // System bell
    printf("Bell from pane %u\n", pane_id);
}

static void on_title_change(uint32_t pane_id, const char* title, void* user_data) {
    printf("Pane %u title: %s\n", pane_id, title);
}

// ============================================================================
// Simulation Functions
// ============================================================================

static void simulate_terminal_output(struct ui_backend* backend) {
    // Simulate typing "Hello, Ghostty!"
    const char* text = "Hello, Ghostty!\n";
    
    struct tty_ctx ctx = {
        .sx = 80,
        .sy = 24,
        .ocx = 0,
        .ocy = 0,
        .num = 1,
        .orupper = 0,
        .orlower = 23
    };
    
    struct grid_cell cell = {
        .attr = 0,
        .fg = 7,
        .bg = 0
    };
    
    for (const char* p = text; *p; p++) {
        cell.codepoint = *p;
        ctx.cell = &cell;
        
        if (*p == '\n') {
            // Line feed
            backend->ops->cmd_linefeed(backend, &ctx);
            ctx.ocx = 0;
            ctx.ocy++;
        } else {
            // Regular character
            backend->ops->cmd_cell(backend, &ctx);
            ctx.ocx++;
        }
        
        usleep(50000);  // 50ms delay for visual effect
    }
}

static void simulate_screen_clear(struct ui_backend* backend) {
    struct tty_ctx ctx = {
        .sx = 80,
        .sy = 24,
        .ocx = 0,
        .ocy = 0
    };
    
    printf("Clearing screen...\n");
    backend->ops->cmd_clearscreen(backend, &ctx);
}

static void simulate_scrolling(struct ui_backend* backend) {
    struct tty_ctx ctx = {
        .sx = 80,
        .sy = 24,
        .orupper = 0,
        .orlower = 23,
        .num = 1
    };
    
    printf("Scrolling up...\n");
    for (int i = 0; i < 5; i++) {
        backend->ops->cmd_scrollup(backend, &ctx);
        usleep(100000);
    }
}

// ============================================================================
// Main Integration
// ============================================================================

int main(int argc, char* argv[]) {
    printf("=== Ghostty Backend Integration Example ===\n\n");
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize hook system
    tty_hooks_init();
    printf("✓ Hook system initialized\n");
    
    // Create Ghostty backend with specific capabilities
    ui_capabilities_t caps = {
        .size = sizeof(ui_capabilities_t),
        .version = UI_BACKEND_ABI_VERSION,
        .supported = UI_CAP_FRAME_BATCH | UI_CAP_24BIT_COLOR | UI_CAP_UTF8_LINES,
        .max_fps = 60,
        .optimal_batch_size = 100,
        .max_dirty_rects = 16
    };
    
    g_state.ghostty_backend = ghostty_backend_create(&caps);
    if (!g_state.ghostty_backend) {
        fprintf(stderr, "Failed to create Ghostty backend\n");
        return 1;
    }
    printf("✓ Ghostty backend created\n");
    
    // Register callbacks
    g_state.ghostty_backend->on_frame = on_frame_received;
    g_state.ghostty_backend->on_bell = on_bell;
    g_state.ghostty_backend->on_title = on_title_change;
    g_state.ghostty_backend->user_data = NULL;
    printf("✓ Callbacks registered\n");
    
    // Create and configure router
    g_state.router = backend_router_create(BACKEND_MODE_UI);
    if (!g_state.router) {
        fprintf(stderr, "Failed to create router\n");
        ghostty_backend_destroy(g_state.ghostty_backend);
        return 1;
    }
    
    // Register backend with router
    int ret = backend_router_register_ui(g_state.router, g_state.ghostty_backend);
    if (ret != 0) {
        fprintf(stderr, "Failed to register backend with router\n");
        backend_router_destroy(g_state.router);
        ghostty_backend_destroy(g_state.ghostty_backend);
        return 1;
    }
    printf("✓ Backend registered with router\n");
    
    // Set as global router
    if (backend_router_init_global(BACKEND_MODE_UI) == 0) {
        global_backend_router = g_state.router;
        printf("✓ Router set as global\n");
    }
    
    // Install hooks to intercept tty_cmd_* functions
    ret = tty_hooks_install(g_state.ghostty_backend);
    if (ret == 0) {
        printf("✓ Hooks installed successfully\n");
    } else {
        printf("⚠ Hook installation failed (continuing anyway)\n");
    }
    
    // Print configuration
    printf("\nConfiguration:\n");
    printf("  Backend type: Ghostty\n");
    printf("  Frame batching: %s\n", 
           (caps.supported & UI_CAP_FRAME_BATCH) ? "enabled" : "disabled");
    printf("  Target FPS: %u\n", caps.max_fps);
    printf("  Batch size: %u\n", caps.optimal_batch_size);
    
    // Run demonstrations
    printf("\n=== Running Demonstrations ===\n\n");
    
    if (argc > 1 && strcmp(argv[1], "--interactive") == 0) {
        printf("Interactive mode - type 'help' for commands\n");
        
        char cmd[256];
        while (g_state.running) {
            printf("> ");
            if (fgets(cmd, sizeof(cmd), stdin) == NULL) break;
            
            // Remove newline
            cmd[strcspn(cmd, "\n")] = 0;
            
            if (strcmp(cmd, "help") == 0) {
                printf("Commands:\n");
                printf("  text     - Simulate text output\n");
                printf("  clear    - Clear screen\n");
                printf("  scroll   - Test scrolling\n");
                printf("  stats    - Show statistics\n");
                printf("  quit     - Exit\n");
            } else if (strcmp(cmd, "text") == 0) {
                simulate_terminal_output(g_state.ghostty_backend);
            } else if (strcmp(cmd, "clear") == 0) {
                simulate_screen_clear(g_state.ghostty_backend);
            } else if (strcmp(cmd, "scroll") == 0) {
                simulate_scrolling(g_state.ghostty_backend);
            } else if (strcmp(cmd, "stats") == 0) {
                uint64_t frames, cells, batched;
                ghostty_backend_get_statistics(g_state.ghostty_backend,
                                              &frames, &cells, &batched);
                printf("Statistics:\n");
                printf("  Frames sent: %llu\n", frames);
                printf("  Cells updated: %llu\n", cells);
                printf("  Frames batched: %llu\n", batched);
            } else if (strcmp(cmd, "quit") == 0) {
                break;
            } else if (strlen(cmd) > 0) {
                printf("Unknown command: %s\n", cmd);
            }
        }
    } else {
        // Automated demonstration
        printf("1. Simulating text output...\n");
        simulate_terminal_output(g_state.ghostty_backend);
        sleep(1);
        
        printf("\n2. Testing screen clear...\n");
        simulate_screen_clear(g_state.ghostty_backend);
        sleep(1);
        
        printf("\n3. Testing scrolling...\n");
        simulate_scrolling(g_state.ghostty_backend);
        sleep(1);
        
        // Show final statistics
        printf("\n=== Final Statistics ===\n");
        uint64_t frames, cells, batched;
        ghostty_backend_get_statistics(g_state.ghostty_backend,
                                      &frames, &cells, &batched);
        printf("  Frames sent: %llu\n", frames);
        printf("  Cells updated: %llu\n", cells);
        printf("  Frames batched: %llu\n", batched);
        
        // Get hook statistics
        tty_hook_stats_t hook_stats;
        tty_hooks_get_stats(&hook_stats);
        printf("\nHook Statistics:\n");
        printf("  Total calls: %llu\n", hook_stats.total_calls);
        printf("  Intercepted: %llu\n", hook_stats.intercepted_calls);
        printf("  Fallback: %llu\n", hook_stats.fallback_calls);
        
        // Show per-function statistics
        printf("\nPer-function calls:\n");
        for (int i = 0; i < tty_hooks_get_count(); i++) {
            if (hook_stats.call_count[i] > 0) {
                printf("  %s: %llu\n", 
                       tty_hooks_get_function_name(i),
                       hook_stats.call_count[i]);
            }
        }
    }
    
    // Cleanup
    printf("\n=== Cleaning up ===\n");
    
    tty_hooks_uninstall();
    printf("✓ Hooks uninstalled\n");
    
    backend_router_unregister_ui(g_state.router);
    printf("✓ Backend unregistered\n");
    
    backend_router_destroy(g_state.router);
    printf("✓ Router destroyed\n");
    
    ghostty_backend_destroy(g_state.ghostty_backend);
    printf("✓ Backend destroyed\n");
    
    printf("\n=== Integration example completed successfully ===\n");
    
    return 0;
}