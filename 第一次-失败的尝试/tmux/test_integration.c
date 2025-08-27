/* test_integration.c - Test the libtmuxcore library */
#include "libtmuxcore_api.h"
#include <stdio.h>
#include <assert.h>

int main() {
    printf("=== Testing libtmuxcore integration ===\n\n");
    
    /* Test 1: Initialize */
    printf("Test 1: Initialize library...\n");
    assert(tmc_init() == TMC_SUCCESS);
    printf("  ✓ Library initialized\n\n");
    
    /* Test 2: Create session */
    printf("Test 2: Create session...\n");
    tmc_session_t session;
    assert(tmc_session_new("test-session", &session) == TMC_SUCCESS);
    printf("  ✓ Session created\n\n");
    
    /* Test 3: Attach session */
    printf("Test 3: Attach session...\n");
    assert(tmc_session_attach(session) == TMC_SUCCESS);
    printf("  ✓ Session attached\n\n");
    
    /* Test 4: Create window */
    printf("Test 4: Create window...\n");
    tmc_window_t window;
    assert(tmc_window_new(session, "test-window", &window) == TMC_SUCCESS);
    printf("  ✓ Window created\n\n");
    
    /* Test 5: Split pane */
    printf("Test 5: Split pane...\n");
    tmc_pane_t pane1, pane2;
    assert(tmc_pane_split(window, 1, 50, &pane1) == TMC_SUCCESS);
    assert(tmc_pane_split(window, 0, 50, &pane2) == TMC_SUCCESS);
    printf("  ✓ Panes split\n\n");
    
    /* Test 6: Execute command */
    printf("Test 6: Execute command...\n");
    assert(tmc_command_execute("list-sessions") == TMC_SUCCESS);
    printf("  ✓ Command executed\n\n");
    
    /* Test 7: Send keys */
    printf("Test 7: Send keys...\n");
    assert(tmc_command_send_keys(pane1, "echo 'Hello from tmux!'") == TMC_SUCCESS);
    printf("  ✓ Keys sent\n\n");
    
    /* Test 8: Register callbacks */
    printf("Test 8: Register callbacks...\n");
    tmc_callbacks_t callbacks = {
        .on_output = NULL,
        .on_bell = NULL,
        .on_title_change = NULL,
        .on_activity = NULL
    };
    assert(tmc_callbacks_register(&callbacks) == TMC_SUCCESS);
    printf("  ✓ Callbacks registered\n\n");
    
    /* Cleanup */
    printf("Test 9: Cleanup...\n");
    tmc_cleanup();
    printf("  ✓ Library cleaned up\n\n");
    
    printf("=== All tests passed! ✅ ===\n");
    return 0;
}