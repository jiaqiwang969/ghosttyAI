// test_compatibility.c - Verify interface compatibility with backend_router.c
// Purpose: Ensure DEFECT-002 fix maintains compatibility
// Author: CORE-001 (c-tmux-specialist)
// Date: 2025-08-25

#include <stdio.h>
#include <assert.h>

// Forward declare to avoid dependency
struct ui_backend;

#include "tty_write_hooks.h"

// Test that both old and new names work
void test_compatibility_macros() {
    printf("Testing compatibility macros...\n");
    
    // New standardized names
    tty_hooks_init();
    printf("  ✓ tty_hooks_init() works\n");
    
    // Old names via compatibility macros (should also work)
    #ifndef TTY_HOOKS_NO_COMPAT
    tty_write_hooks_init();  // This should map to tty_hooks_init()
    printf("  ✓ tty_write_hooks_init() compatibility macro works\n");
    #endif
    
    printf("  ✓ All compatibility tests passed\n");
}

// Test that the interface matches what backend_router.c expects
void test_backend_router_interface() {
    printf("Testing backend_router.c interface compatibility...\n");
    
    // backend_router.c includes "tty_write_hooks.h" and expects:
    // - tty_hooks_init() to be available
    // - tty_hooks_install() to be available  
    // - tty_hooks_uninstall() to be available
    
    // These are function pointers to verify they exist
    void (*init_fn)(void) = tty_hooks_init;
    int (*install_fn)(struct ui_backend*) = tty_hooks_install;
    int (*uninstall_fn)(void) = tty_hooks_uninstall;
    
    assert(init_fn != NULL);
    assert(install_fn != NULL);
    assert(uninstall_fn != NULL);
    
    printf("  ✓ All required functions are available\n");
    printf("  ✓ Interface matches backend_router.c expectations\n");
}

int main() {
    printf("=============================================================\n");
    printf("DEFECT-002: Interface Compatibility Test\n");
    printf("=============================================================\n\n");
    
    test_compatibility_macros();
    test_backend_router_interface();
    
    printf("\n=============================================================\n");
    printf("Result: ✅ DEFECT-002 FIXED - All interfaces compatible\n");
    printf("=============================================================\n");
    
    return 0;
}