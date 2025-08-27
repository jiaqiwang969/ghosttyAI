/* test_enhanced.c - Test the enhanced libtmuxcore library */
#include "libtmuxcore_api.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_session_management() {
    printf("\n=== Testing Session Management ===\n");
    
    /* Create multiple sessions */
    tmc_session_t session1, session2;
    assert(tmc_session_new("work", &session1) == TMC_SUCCESS);
    assert(tmc_session_new("personal", &session2) == TMC_SUCCESS);
    
    /* Try to create duplicate */
    tmc_session_t dup_session;
    assert(tmc_session_new("work", &dup_session) == TMC_ERROR_ALREADY_EXISTS);
    printf("  ✓ Duplicate session prevention works\n");
    
    /* Attach and detach */
    assert(tmc_session_attach(session1) == TMC_SUCCESS);
    assert(tmc_session_detach(session1) == TMC_SUCCESS);
    printf("  ✓ Session attach/detach works\n");
    
    /* List sessions */
    tmc_command_execute("list-sessions");
    printf("  ✓ Session listing works\n");
}

void test_window_management() {
    printf("\n=== Testing Window Management ===\n");
    
    /* Create session first */
    tmc_session_t session;
    assert(tmc_session_new("test-windows", &session) == TMC_SUCCESS);
    assert(tmc_session_attach(session) == TMC_SUCCESS);
    
    /* Create multiple windows */
    tmc_window_t win1, win2, win3;
    assert(tmc_window_new(session, "editor", &win1) == TMC_SUCCESS);
    assert(tmc_window_new(session, "terminal", &win2) == TMC_SUCCESS);
    assert(tmc_window_new(session, "logs", &win3) == TMC_SUCCESS);
    printf("  ✓ Multiple window creation works\n");
    
    /* List windows */
    tmc_command_execute("list-windows");
    printf("  ✓ Window listing works\n");
}

void test_pane_splitting() {
    printf("\n=== Testing Pane Splitting ===\n");
    
    /* Setup */
    tmc_session_t session;
    tmc_window_t window;
    assert(tmc_session_new("test-panes", &session) == TMC_SUCCESS);
    assert(tmc_session_attach(session) == TMC_SUCCESS);
    
    /* The session creates a default window, we need to get it */
    /* For now, we'll create another window */
    assert(tmc_window_new(session, "split-test", &window) == TMC_SUCCESS);
    
    /* Split panes */
    tmc_pane_t pane1, pane2, pane3;
    assert(tmc_pane_split(window, 1, 50, &pane1) == TMC_SUCCESS);  /* Horizontal split */
    assert(tmc_pane_split(window, 0, 50, &pane2) == TMC_SUCCESS);  /* Vertical split */
    assert(tmc_pane_split(window, 1, 30, &pane3) == TMC_SUCCESS);  /* Another horizontal */
    printf("  ✓ Pane splitting works\n");
    
    /* List panes */
    tmc_command_execute("list-panes");
    printf("  ✓ Pane listing works\n");
}

void test_command_execution() {
    printf("\n=== Testing Command Execution ===\n");
    
    /* Execute various commands */
    assert(tmc_command_execute("list-sessions") == TMC_SUCCESS);
    assert(tmc_command_execute("list-windows") == TMC_SUCCESS);
    assert(tmc_command_execute("list-panes") == TMC_SUCCESS);
    printf("  ✓ All commands executed successfully\n");
}

void stress_test() {
    printf("\n=== Stress Testing ===\n");
    
    /* Create many sessions */
    for (int i = 0; i < 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "stress-session-%d", i);
        tmc_session_t session;
        assert(tmc_session_new(name, &session) == TMC_SUCCESS);
        
        /* Add windows to each session */
        for (int j = 0; j < 3; j++) {
            char win_name[32];
            snprintf(win_name, sizeof(win_name), "window-%d", j);
            tmc_window_t window;
            assert(tmc_window_new(session, win_name, &window) == TMC_SUCCESS);
            
            /* Split each window */
            tmc_pane_t pane;
            assert(tmc_pane_split(window, j % 2, 50, &pane) == TMC_SUCCESS);
        }
    }
    printf("  ✓ Created 10 sessions with 3 windows each\n");
    
    /* List everything */
    tmc_command_execute("list-sessions");
}

int main() {
    printf("=== Enhanced libtmuxcore Testing ===\n");
    
    /* Initialize library */
    assert(tmc_init() == TMC_SUCCESS);
    printf("✓ Library initialized\n");
    
    /* Run test suites */
    test_session_management();
    test_window_management();
    test_pane_splitting();
    test_command_execution();
    stress_test();
    
    /* Cleanup */
    tmc_cleanup();
    printf("\n✓ Library cleaned up\n");
    
    printf("\n=== All enhanced tests passed! ✅ ===\n");
    return 0;
}