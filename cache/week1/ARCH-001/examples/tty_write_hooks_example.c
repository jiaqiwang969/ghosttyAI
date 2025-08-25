// tty_write_hooks_example.c - Example Implementation for CORE-001
// Author: ARCH-001 (System Architect)
// Purpose: Show how to use the fixed tty_ctx structure and interfaces
// Version: 2.0.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../tty_ctx_unified.h"
#include "../interface_adapter.h"

// ============================================================================
// Hook Function Definitions
// ============================================================================

// Hook that processes tty_write commands
static int hook_tty_write(struct tty_ctx* ctx, int cmd) {
    // Validate context first
    if (!tty_ctx_is_valid(ctx)) {
        fprintf(stderr, "[Hook] Invalid tty_ctx received\n");
        return -1;
    }
    
    // Now we can safely access all fields
    uint32_t x = TTY_CTX_GET_OCX(ctx);
    uint32_t y = TTY_CTX_GET_OCY(ctx);
    uint32_t upper = TTY_CTX_GET_FIELD(ctx, orupper, 0);
    uint32_t lower = TTY_CTX_GET_FIELD(ctx, orlower, 24);
    
    printf("[Hook] Processing cmd=%d at position (%u,%u) region=[%u,%u]\n",
           cmd, x, y, upper, lower);
    
    // Check if we have cell data
    void* cell = TTY_CTX_GET_FIELD(ctx, cell, NULL);
    if (cell) {
        printf("[Hook] Cell data present\n");
    }
    
    // Check window pointer
    void* wp = TTY_CTX_GET_FIELD(ctx, wp, NULL);
    if (wp) {
        printf("[Hook] Window pointer present\n");
    }
    
    return 0;
}

// Hook for scroll operations
static int hook_scroll(struct tty_ctx* ctx, int direction, int lines) {
    if (!tty_ctx_is_valid(ctx)) {
        return -1;
    }
    
    uint32_t upper = TTY_CTX_GET_FIELD(ctx, orupper, 0);
    uint32_t lower = TTY_CTX_GET_FIELD(ctx, orlower, 0);
    
    printf("[Hook] Scroll %s by %d lines in region [%u,%u]\n",
           direction > 0 ? "down" : "up", lines, upper, lower);
    
    return 0;
}

// ============================================================================
// Hook Registration Example
// ============================================================================

typedef struct {
    const char* name;
    void* hook_fn;
} hook_entry_t;

static hook_entry_t registered_hooks[32];
static int hook_count = 0;

int register_tty_hooks(void) {
    printf("Registering TTY hooks...\n");
    
    // Initialize the hook system using standard interface
    int result = tty_hooks_init();
    if (result != 0) {
        fprintf(stderr, "Failed to initialize hook system\n");
        return -1;
    }
    
    // Register our hooks
    registered_hooks[hook_count].name = "tty_write";
    registered_hooks[hook_count].hook_fn = hook_tty_write;
    hook_count++;
    
    registered_hooks[hook_count].name = "scroll";
    registered_hooks[hook_count].hook_fn = hook_scroll;
    hook_count++;
    
    // Install hooks using the interface
    for (int i = 0; i < hook_count; i++) {
        result = tty_hooks_install(registered_hooks[i].name, 
                                  registered_hooks[i].hook_fn);
        if (result != 0) {
            fprintf(stderr, "Failed to install hook: %s\n", 
                    registered_hooks[i].name);
        } else {
            printf("  ✓ Installed hook: %s\n", registered_hooks[i].name);
        }
    }
    
    return 0;
}

// ============================================================================
// Migration Example: Old Code vs New Code
// ============================================================================

void demonstrate_migration(void) {
    printf("\n=== Migration Example ===\n");
    
    // OLD CODE (would fail compilation):
    /*
    struct tty_ctx old_ctx;
    old_ctx.cell = some_cell;
    old_ctx.ocx = 10;  // ERROR: field doesn't exist in old struct
    */
    
    // NEW CODE (works correctly):
    struct tty_ctx new_ctx = {0};
    tty_ctx_init(&new_ctx);
    
    // Set fields safely
    TTY_CTX_SET_FIELD(&new_ctx, ocx, 10);
    TTY_CTX_SET_FIELD(&new_ctx, ocy, 20);
    TTY_CTX_SET_FIELD(&new_ctx, orupper, 0);
    TTY_CTX_SET_FIELD(&new_ctx, orlower, 24);
    
    // Simulate setting cell data
    ui_cell_t cell = {
        .data = {'X', 0, 0, 0},
        .attr = {.fg = 0xFFFFFF, .bg = 0x000000}
    };
    new_ctx.cell = &cell;
    
    // Call hook with new context
    hook_tty_write(&new_ctx, 1);
}

// ============================================================================
// Test Scenarios
// ============================================================================

void test_edge_cases(void) {
    printf("\n=== Testing Edge Cases ===\n");
    
    // Test 1: NULL context
    printf("\nTest 1: NULL context handling\n");
    int result = hook_tty_write(NULL, 1);
    printf("  Result: %d (expected: -1)\n", result);
    
    // Test 2: Invalid context (wrong size)
    printf("\nTest 2: Invalid context size\n");
    struct {
        uint32_t size;
        uint32_t version;
    } invalid_ctx = {10, 1};
    result = hook_tty_write((struct tty_ctx*)&invalid_ctx, 1);
    printf("  Result: %d (expected: -1)\n", result);
    
    // Test 3: Migration from old context
    printf("\nTest 3: Context migration\n");
    struct tty_ctx old_style = {0};
    old_style.size = 24; // Old size
    old_style.version = 0;
    
    result = tty_ctx_migrate(&old_style);
    printf("  Migration result: %d\n", result);
    printf("  New size: %u (expected: %lu)\n", 
           old_style.size, sizeof(struct tty_ctx));
    printf("  New version: %u (expected: %d)\n", 
           old_style.version, TTY_CTX_VERSION_CURRENT);
}

// ============================================================================
// Main Example
// ============================================================================

int main(void) {
    printf("=== TTY Write Hooks Integration Example ===\n");
    printf("For: CORE-001 (c-tmux-specialist)\n\n");
    
    // Step 1: Register hooks
    printf("Step 1: Registering hooks\n");
    if (register_tty_hooks() != 0) {
        fprintf(stderr, "Failed to register hooks\n");
        return 1;
    }
    
    // Step 2: Demonstrate migration
    printf("\nStep 2: Demonstrating migration\n");
    demonstrate_migration();
    
    // Step 3: Test edge cases
    printf("\nStep 3: Testing edge cases\n");
    test_edge_cases();
    
    // Step 4: Test compatibility layer
    printf("\nStep 4: Testing compatibility layer\n");
    printf("Calling deprecated function tty_write_hooks_init()...\n");
    int result = tty_write_hooks_init();
    printf("  Result: %d (should work with warning)\n", result);
    
    // Cleanup
    printf("\nStep 5: Cleanup\n");
    tty_hooks_cleanup();
    
    printf("\n✅ TTY hooks example completed successfully!\n");
    return 0;
}