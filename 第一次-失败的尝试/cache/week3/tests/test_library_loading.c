// test_library_loading.c - Test dynamic library loading for libtmuxcore
// Task: T-305-R - End-to-End Integration Testing
// Purpose: Validate libtmuxcore.dylib can be loaded and symbols resolved
// Author: week3-ghostty-tmux-executor
// Date: 2025-08-26

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <assert.h>
#include <string.h>

#define TEST_ASSERT(cond, msg) \
    if (!(cond)) { \
        fprintf(stderr, "❌ Test failed: %s\n  %s:%d\n", msg, __FILE__, __LINE__); \
        return 1; \
    }

#define TEST_PASS(name) printf("✓ %s\n", name)

// Function pointer types for libtmuxcore API
typedef void* (*tmc_init_fn)(void);
typedef void (*tmc_cleanup_fn)(void*);
typedef void (*tmc_get_version_fn)(int*, int*, int*);
typedef int (*tmc_create_session_fn)(void*, const char*);
typedef int (*tmc_destroy_session_fn)(void*, const char*);
typedef int (*tmc_execute_command_fn)(void*, const char*);
typedef int (*tmc_set_backend_mode_fn)(void*, const char*);

// UI callback structure matching libtmuxcore.h
typedef struct {
    void (*on_redraw)(void*);
    void (*on_cell_update)(int, int, const char*, void*);
    void (*on_cursor_move)(int, int, void*);
    void (*on_resize)(int, int, void*);
    void* user_data;
} test_ui_callbacks_t;

typedef int (*tmc_register_callbacks_fn)(void*, const test_ui_callbacks_t*);

// Test callbacks to verify they get called
static int callback_redraw_count = 0;
static int callback_cell_count = 0;
static int callback_cursor_count = 0;
static int callback_resize_count = 0;

void test_on_redraw(void* user_data) {
    callback_redraw_count++;
    printf("  Callback: redraw called (%d times)\n", callback_redraw_count);
}

void test_on_cell_update(int row, int col, const char* text, void* user_data) {
    callback_cell_count++;
    printf("  Callback: cell update at (%d,%d) = '%s'\n", row, col, text ? text : "(null)");
}

void test_on_cursor_move(int row, int col, void* user_data) {
    callback_cursor_count++;
    printf("  Callback: cursor move to (%d,%d)\n", row, col);
}

void test_on_resize(int rows, int cols, void* user_data) {
    callback_resize_count++;
    printf("  Callback: resize to %dx%d\n", rows, cols);
}

int test_library_loading() {
    printf("\n=== Testing Dynamic Library Loading ===\n");
    
    // 1. Test library loading
    void* handle = dlopen("./libtmuxcore.dylib", RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        // Try tmux directory
        handle = dlopen("/Users/jqwang/98-ghosttyAI/tmux/libtmuxcore.dylib", RTLD_LAZY | RTLD_LOCAL);
    }
    
    TEST_ASSERT(handle != NULL, dlerror());
    TEST_PASS("Library loaded successfully");
    
    // 2. Test symbol resolution - Core functions
    tmc_init_fn tmc_init = dlsym(handle, "tmc_init");
    TEST_ASSERT(tmc_init != NULL, "Failed to resolve tmc_init");
    TEST_PASS("tmc_init resolved");
    
    tmc_cleanup_fn tmc_cleanup = dlsym(handle, "tmc_cleanup");
    TEST_ASSERT(tmc_cleanup != NULL, "Failed to resolve tmc_cleanup");
    TEST_PASS("tmc_cleanup resolved");
    
    tmc_get_version_fn tmc_get_version = dlsym(handle, "tmc_get_version");
    TEST_ASSERT(tmc_get_version != NULL, "Failed to resolve tmc_get_version");
    TEST_PASS("tmc_get_version resolved");
    
    // 3. Test version API
    int major = 0, minor = 0, patch = 0;
    tmc_get_version(&major, &minor, &patch);
    printf("  Library version: %d.%d.%d\n", major, minor, patch);
    TEST_ASSERT(major == 1 && minor == 0 && patch == 0, "Unexpected version");
    TEST_PASS("Version API works");
    
    // 4. Test initialization
    void* tmux_handle = tmc_init();
    TEST_ASSERT(tmux_handle != NULL, "Failed to initialize tmux core");
    TEST_PASS("Core initialized");
    
    // 5. Test session management
    tmc_create_session_fn tmc_create_session = dlsym(handle, "tmc_create_session");
    TEST_ASSERT(tmc_create_session != NULL, "Failed to resolve tmc_create_session");
    
    int ret = tmc_create_session(tmux_handle, "test-session");
    TEST_ASSERT(ret == 0, "Failed to create session");
    TEST_PASS("Session created");
    
    // 6. Test callback registration
    tmc_register_callbacks_fn tmc_register_callbacks = dlsym(handle, "tmc_register_callbacks");
    TEST_ASSERT(tmc_register_callbacks != NULL, "Failed to resolve tmc_register_callbacks");
    
    test_ui_callbacks_t callbacks = {
        .on_redraw = test_on_redraw,
        .on_cell_update = test_on_cell_update,
        .on_cursor_move = test_on_cursor_move,
        .on_resize = test_on_resize,
        .user_data = NULL
    };
    
    ret = tmc_register_callbacks(tmux_handle, &callbacks);
    TEST_ASSERT(ret == 0, "Failed to register callbacks");
    TEST_PASS("Callbacks registered");
    
    // 7. Test command execution
    tmc_execute_command_fn tmc_execute_command = dlsym(handle, "tmc_execute_command");
    TEST_ASSERT(tmc_execute_command != NULL, "Failed to resolve tmc_execute_command");
    
    ret = tmc_execute_command(tmux_handle, "list-sessions");
    TEST_ASSERT(ret == 0, "Failed to execute command");
    TEST_PASS("Command executed");
    
    // 8. Test backend mode setting
    tmc_set_backend_mode_fn tmc_set_backend_mode = dlsym(handle, "tmc_set_backend_mode");
    TEST_ASSERT(tmc_set_backend_mode != NULL, "Failed to resolve tmc_set_backend_mode");
    
    ret = tmc_set_backend_mode(tmux_handle, "ghostty");
    TEST_ASSERT(ret == 0, "Failed to set backend mode");
    TEST_PASS("Backend mode set");
    
    // 9. Test session destruction
    tmc_destroy_session_fn tmc_destroy_session = dlsym(handle, "tmc_destroy_session");
    TEST_ASSERT(tmc_destroy_session != NULL, "Failed to resolve tmc_destroy_session");
    
    ret = tmc_destroy_session(tmux_handle, "test-session");
    TEST_ASSERT(ret == 0, "Failed to destroy session");
    TEST_PASS("Session destroyed");
    
    // 10. Cleanup
    tmc_cleanup(tmux_handle);
    TEST_PASS("Core cleaned up");
    
    dlclose(handle);
    TEST_PASS("Library unloaded");
    
    printf("\n✅ All library loading tests passed!\n");
    printf("  Callbacks triggered: redraw=%d, cell=%d, cursor=%d, resize=%d\n",
           callback_redraw_count, callback_cell_count, 
           callback_cursor_count, callback_resize_count);
    
    return 0;
}

int main(int argc, char* argv[]) {
    printf("libtmuxcore Dynamic Library Test Suite\n");
    printf("======================================\n");
    
    int result = test_library_loading();
    if (result != 0) {
        printf("\n❌ Tests failed with code %d\n", result);
        return result;
    }
    
    printf("\n✅ All tests passed successfully!\n");
    return 0;
}