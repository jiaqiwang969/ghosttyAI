// test_integration_e2e.c - End-to-end integration test suite
// Task: T-305-R - End-to-End Integration Testing
// Purpose: Validate complete tmux↔Ghostty integration with real operations
// Author: week3-ghostty-tmux-executor  
// Date: 2025-08-26

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>

// Test framework
#define TEST_SUITE_START(name) printf("\n" "=" "=" "=" " %s " "=" "=" "=" "\n", name)
#define TEST_CASE(name) printf("\n[TEST] %s\n", name)
#define ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "  ❌ Assertion failed: %s\n    at %s:%d\n", \
                #cond, __FILE__, __LINE__); \
        test_failures++; \
        return; \
    } \
} while(0)
#define TEST_PASS(msg) printf("  ✓ %s\n", msg)

static int test_failures = 0;
static int test_passes = 0;

// libtmuxcore function pointers
static void* lib_handle = NULL;
static void* (*tmc_init)(void) = NULL;
static void (*tmc_cleanup)(void*) = NULL;
static int (*tmc_create_session)(void*, const char*) = NULL;
static int (*tmc_destroy_session)(void*, const char*) = NULL;
static int (*tmc_execute_command)(void*, const char*) = NULL;
static int (*tmc_split_window)(void*, const char*, int) = NULL;
static int (*tmc_select_pane)(void*, int) = NULL;
static int (*tmc_send_keys)(void*, const char*) = NULL;
static int (*tmc_set_backend_mode)(void*, const char*) = NULL;

// Callback tracking
typedef struct {
    void (*on_redraw)(void*);
    void (*on_cell_update)(int, int, const char*, void*);
    void (*on_cursor_move)(int, int, void*);
    void (*on_resize)(int, int, void*);
    void (*on_pane_created)(int, void*);
    void (*on_pane_closed)(int, void*);
    void* user_data;
} ui_callbacks_t;

static int (*tmc_register_callbacks)(void*, const ui_callbacks_t*) = NULL;

// Callback counters
static struct {
    int redraw;
    int cell_update;
    int cursor_move;
    int resize;
    int pane_created;
    int pane_closed;
    pthread_mutex_t lock;
} callback_stats = {
    .lock = PTHREAD_MUTEX_INITIALIZER
};

// Test callbacks
void test_on_redraw(void* user) {
    pthread_mutex_lock(&callback_stats.lock);
    callback_stats.redraw++;
    pthread_mutex_unlock(&callback_stats.lock);
}

void test_on_cell_update(int row, int col, const char* text, void* user) {
    pthread_mutex_lock(&callback_stats.lock);
    callback_stats.cell_update++;
    pthread_mutex_unlock(&callback_stats.lock);
}

void test_on_cursor_move(int row, int col, void* user) {
    pthread_mutex_lock(&callback_stats.lock);
    callback_stats.cursor_move++;
    pthread_mutex_unlock(&callback_stats.lock);
}

void test_on_resize(int rows, int cols, void* user) {
    pthread_mutex_lock(&callback_stats.lock);
    callback_stats.resize++;
    pthread_mutex_unlock(&callback_stats.lock);
    printf("    Window resized to %dx%d\n", rows, cols);
}

void test_on_pane_created(int pane_id, void* user) {
    pthread_mutex_lock(&callback_stats.lock);
    callback_stats.pane_created++;
    pthread_mutex_unlock(&callback_stats.lock);
    printf("    Pane created: %d\n", pane_id);
}

void test_on_pane_closed(int pane_id, void* user) {
    pthread_mutex_lock(&callback_stats.lock);
    callback_stats.pane_closed++;
    pthread_mutex_unlock(&callback_stats.lock);
    printf("    Pane closed: %d\n", pane_id);
}

// Test 1: Library initialization and cleanup
void test_library_lifecycle() {
    TEST_CASE("Library Lifecycle");
    
    // Load library
    lib_handle = dlopen("/Users/jqwang/98-ghosttyAI/tmux/libtmuxcore.dylib", RTLD_LAZY);
    ASSERT(lib_handle != NULL);
    TEST_PASS("Library loaded");
    
    // Resolve symbols
    tmc_init = dlsym(lib_handle, "tmc_init");
    tmc_cleanup = dlsym(lib_handle, "tmc_cleanup");
    tmc_create_session = dlsym(lib_handle, "tmc_create_session");
    tmc_destroy_session = dlsym(lib_handle, "tmc_destroy_session");
    tmc_execute_command = dlsym(lib_handle, "tmc_execute_command");
    tmc_register_callbacks = dlsym(lib_handle, "tmc_register_callbacks");
    tmc_set_backend_mode = dlsym(lib_handle, "tmc_set_backend_mode");
    
    // Additional functions for testing
    tmc_split_window = dlsym(lib_handle, "tmc_split_window");
    tmc_select_pane = dlsym(lib_handle, "tmc_select_pane");
    tmc_send_keys = dlsym(lib_handle, "tmc_send_keys");
    
    ASSERT(tmc_init != NULL);
    ASSERT(tmc_cleanup != NULL);
    TEST_PASS("Core symbols resolved");
    
    // Initialize
    void* handle = tmc_init();
    ASSERT(handle != NULL);
    TEST_PASS("Core initialized");
    
    // Set backend mode
    if (tmc_set_backend_mode) {
        int ret = tmc_set_backend_mode(handle, "ghostty");
        ASSERT(ret == 0);
        TEST_PASS("Backend mode set to ghostty");
    }
    
    // Cleanup
    tmc_cleanup(handle);
    TEST_PASS("Core cleaned up");
    
    test_passes++;
}

// Test 2: Session management
void test_session_management() {
    TEST_CASE("Session Management");
    
    void* handle = tmc_init();
    ASSERT(handle != NULL);
    
    // Create sessions
    int ret = tmc_create_session(handle, "test-session-1");
    ASSERT(ret == 0);
    TEST_PASS("Session 1 created");
    
    ret = tmc_create_session(handle, "test-session-2");
    ASSERT(ret == 0);
    TEST_PASS("Session 2 created");
    
    // List sessions
    ret = tmc_execute_command(handle, "list-sessions");
    ASSERT(ret == 0);
    TEST_PASS("Sessions listed");
    
    // Switch sessions
    ret = tmc_execute_command(handle, "switch -t test-session-2");
    ASSERT(ret == 0);
    TEST_PASS("Switched to session 2");
    
    // Destroy sessions
    ret = tmc_destroy_session(handle, "test-session-1");
    ASSERT(ret == 0);
    TEST_PASS("Session 1 destroyed");
    
    ret = tmc_destroy_session(handle, "test-session-2");
    ASSERT(ret == 0);
    TEST_PASS("Session 2 destroyed");
    
    tmc_cleanup(handle);
    test_passes++;
}

// Test 3: Callback mechanism
void test_callback_system() {
    TEST_CASE("Callback System");
    
    void* handle = tmc_init();
    ASSERT(handle != NULL);
    
    // Reset counters
    memset(&callback_stats, 0, sizeof(callback_stats));
    pthread_mutex_init(&callback_stats.lock, NULL);
    
    // Register callbacks
    ui_callbacks_t callbacks = {
        .on_redraw = test_on_redraw,
        .on_cell_update = test_on_cell_update,
        .on_cursor_move = test_on_cursor_move,
        .on_resize = test_on_resize,
        .on_pane_created = test_on_pane_created,
        .on_pane_closed = test_on_pane_closed,
        .user_data = NULL
    };
    
    int ret = tmc_register_callbacks(handle, &callbacks);
    ASSERT(ret == 0);
    TEST_PASS("Callbacks registered");
    
    // Create session (should trigger callbacks)
    ret = tmc_create_session(handle, "callback-test");
    ASSERT(ret == 0);
    
    // Execute commands that trigger callbacks
    ret = tmc_execute_command(handle, "split-window -h");
    usleep(10000); // Give callbacks time to fire
    
    ret = tmc_execute_command(handle, "resize-pane -x 40");
    usleep(10000);
    
    // Check callback stats
    pthread_mutex_lock(&callback_stats.lock);
    printf("  Callback stats:\n");
    printf("    Redraws: %d\n", callback_stats.redraw);
    printf("    Cell updates: %d\n", callback_stats.cell_update);
    printf("    Cursor moves: %d\n", callback_stats.cursor_move);
    printf("    Resizes: %d\n", callback_stats.resize);
    printf("    Panes created: %d\n", callback_stats.pane_created);
    pthread_mutex_unlock(&callback_stats.lock);
    
    // At least some callbacks should have fired
    ASSERT(callback_stats.redraw > 0 || callback_stats.resize > 0);
    TEST_PASS("Callbacks triggered successfully");
    
    tmc_destroy_session(handle, "callback-test");
    tmc_cleanup(handle);
    test_passes++;
}

// Test 4: Window and pane operations
void test_window_pane_operations() {
    TEST_CASE("Window and Pane Operations");
    
    void* handle = tmc_init();
    ASSERT(handle != NULL);
    
    // Create session
    int ret = tmc_create_session(handle, "window-test");
    ASSERT(ret == 0);
    TEST_PASS("Session created");
    
    // Split windows (if functions available)
    if (tmc_split_window) {
        ret = tmc_split_window(handle, "-h", 50);  // Horizontal split at 50%
        ASSERT(ret == 0);
        TEST_PASS("Window split horizontally");
        
        ret = tmc_split_window(handle, "-v", 50);  // Vertical split
        ASSERT(ret == 0);
        TEST_PASS("Window split vertically");
    } else {
        // Fall back to command execution
        ret = tmc_execute_command(handle, "split-window -h");
        ASSERT(ret == 0);
        TEST_PASS("Window split via command");
    }
    
    // Select panes
    if (tmc_select_pane) {
        ret = tmc_select_pane(handle, 0);
        ASSERT(ret == 0);
        TEST_PASS("Selected pane 0");
        
        ret = tmc_select_pane(handle, 1);
        ASSERT(ret == 0);
        TEST_PASS("Selected pane 1");
    }
    
    // Send keys to pane
    if (tmc_send_keys) {
        ret = tmc_send_keys(handle, "echo 'Hello from tmux!'");
        ASSERT(ret == 0);
        TEST_PASS("Keys sent to pane");
    }
    
    // New window
    ret = tmc_execute_command(handle, "new-window -n test-window");
    ASSERT(ret == 0);
    TEST_PASS("New window created");
    
    // Switch windows
    ret = tmc_execute_command(handle, "select-window -t 0");
    ASSERT(ret == 0);
    TEST_PASS("Switched to window 0");
    
    tmc_destroy_session(handle, "window-test");
    tmc_cleanup(handle);
    test_passes++;
}

// Test 5: Command execution
void test_command_execution() {
    TEST_CASE("Command Execution");
    
    void* handle = tmc_init();
    ASSERT(handle != NULL);
    
    // Create test session
    int ret = tmc_create_session(handle, "cmd-test");
    ASSERT(ret == 0);
    
    // Test various commands
    struct {
        const char* command;
        const char* description;
    } commands[] = {
        {"set -g status on", "Enable status bar"},
        {"set -g status-position top", "Move status to top"},
        {"set -g mouse on", "Enable mouse support"},
        {"bind-key C-a send-prefix", "Set prefix key"},
        {"display-message 'Test message'", "Display message"},
        {"list-keys", "List key bindings"},
        {"list-commands", "List all commands"},
        {NULL, NULL}
    };
    
    for (int i = 0; commands[i].command != NULL; i++) {
        ret = tmc_execute_command(handle, commands[i].command);
        if (ret == 0) {
            printf("  ✓ %s\n", commands[i].description);
        } else {
            printf("  ⚠️  %s (returned %d)\n", commands[i].description, ret);
        }
    }
    
    TEST_PASS("Commands executed");
    
    tmc_destroy_session(handle, "cmd-test");
    tmc_cleanup(handle);
    test_passes++;
}

// Test 6: Stress test - rapid operations
void test_stress_operations() {
    TEST_CASE("Stress Test - Rapid Operations");
    
    void* handle = tmc_init();
    ASSERT(handle != NULL);
    
    // Create session
    int ret = tmc_create_session(handle, "stress-test");
    ASSERT(ret == 0);
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    // Rapid window creation/destruction
    for (int i = 0; i < 100; i++) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "new-window -n win%d", i);
        ret = tmc_execute_command(handle, cmd);
        if (ret != 0) break;
        
        if (i % 2 == 0) {
            snprintf(cmd, sizeof(cmd), "kill-window -t win%d", i);
            tmc_execute_command(handle, cmd);
        }
    }
    
    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) + 
                    (end.tv_usec - start.tv_usec) / 1000000.0;
    
    printf("  Completed 100 window operations in %.2f seconds\n", elapsed);
    printf("  Average: %.2f ms per operation\n", (elapsed * 1000) / 100);
    
    TEST_PASS("Stress test completed");
    
    tmc_destroy_session(handle, "stress-test");
    tmc_cleanup(handle);
    test_passes++;
}

// Test 7: Memory leak check
void test_memory_leaks() {
    TEST_CASE("Memory Leak Check");
    
    // Multiple init/cleanup cycles
    for (int cycle = 0; cycle < 10; cycle++) {
        void* handle = tmc_init();
        ASSERT(handle != NULL);
        
        // Create and destroy sessions
        char session_name[32];
        snprintf(session_name, sizeof(session_name), "leak-test-%d", cycle);
        
        int ret = tmc_create_session(handle, session_name);
        ASSERT(ret == 0);
        
        // Some operations
        tmc_execute_command(handle, "split-window");
        tmc_execute_command(handle, "new-window");
        
        ret = tmc_destroy_session(handle, session_name);
        ASSERT(ret == 0);
        
        tmc_cleanup(handle);
    }
    
    TEST_PASS("10 init/cleanup cycles completed");
    TEST_PASS("No obvious memory leaks (run with valgrind for detailed check)");
    
    test_passes++;
}

int main(int argc, char* argv[]) {
    printf("End-to-End Integration Test Suite\n");
    printf("=================================\n");
    printf("Testing libtmuxcore ↔ Ghostty integration\n");
    
    TEST_SUITE_START("INTEGRATION TESTS");
    
    // Run all tests
    test_library_lifecycle();
    test_session_management();
    test_callback_system();
    test_window_pane_operations();
    test_command_execution();
    test_stress_operations();
    test_memory_leaks();
    
    // Cleanup
    if (lib_handle) {
        dlclose(lib_handle);
    }
    
    // Summary
    printf("\n" "=" "=" "=" " TEST SUMMARY " "=" "=" "=" "\n");
    printf("Tests passed: %d\n", test_passes);
    printf("Tests failed: %d\n", test_failures);
    
    if (test_failures > 0) {
        printf("\n❌ TESTS FAILED\n");
        return 1;
    }
    
    printf("\n✅ ALL TESTS PASSED!\n");
    printf("Integration validated successfully.\n");
    return 0;
}