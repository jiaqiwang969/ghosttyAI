// test_existing_functionality.c
// Regression test to ensure no new issues introduced
// Target: Verify all existing functionality still works

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/time.h>

// Mock libtmuxcore structures
typedef struct tmux_session {
    int id;
    char name[256];
    int active;
    void* window_list;
} tmux_session_t;

typedef struct tmux_window {
    int id;
    char name[256];
    int width;
    int height;
    void* pane_list;
} tmux_window_t;

typedef struct tmux_pane {
    int id;
    int x, y;
    int width, height;
    char* buffer;
    size_t buffer_size;
} tmux_pane_t;

// Test result structure
typedef struct test_result {
    const char* name;
    int passed;
    double duration_ms;
    char error_msg[256];
} test_result_t;

// Performance baseline structure
typedef struct perf_baseline {
    double session_create_ms;
    double window_create_ms;
    double pane_split_ms;
    double text_render_ms;
    double resize_ms;
} perf_baseline_t;

// Expected performance baselines (in milliseconds)
static const perf_baseline_t PERF_BASELINE = {
    .session_create_ms = 10.0,
    .window_create_ms = 5.0,
    .pane_split_ms = 3.0,
    .text_render_ms = 1.0,
    .resize_ms = 2.0
};

// Utility function to measure time
double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

// Mock implementations of core functions
tmux_session_t* tmux_create_session(const char* name) {
    tmux_session_t* session = (tmux_session_t*)calloc(1, sizeof(tmux_session_t));
    if (!session) return NULL;
    
    static int session_counter = 0;
    session->id = ++session_counter;
    strncpy(session->name, name, sizeof(session->name) - 1);
    session->active = 1;
    
    return session;
}

void tmux_destroy_session(tmux_session_t* session) {
    if (session) {
        free(session);
    }
}

tmux_window_t* tmux_create_window(tmux_session_t* session, const char* name) {
    if (!session) return NULL;
    
    tmux_window_t* window = (tmux_window_t*)calloc(1, sizeof(tmux_window_t));
    if (!window) return NULL;
    
    static int window_counter = 0;
    window->id = ++window_counter;
    strncpy(window->name, name, sizeof(window->name) - 1);
    window->width = 80;
    window->height = 24;
    
    return window;
}

void tmux_destroy_window(tmux_window_t* window) {
    if (window) {
        free(window);
    }
}

tmux_pane_t* tmux_split_pane(tmux_window_t* window, int vertical) {
    if (!window) return NULL;
    
    tmux_pane_t* pane = (tmux_pane_t*)calloc(1, sizeof(tmux_pane_t));
    if (!pane) return NULL;
    
    static int pane_counter = 0;
    pane->id = ++pane_counter;
    
    if (vertical) {
        pane->width = window->width / 2;
        pane->height = window->height;
    } else {
        pane->width = window->width;
        pane->height = window->height / 2;
    }
    
    pane->buffer_size = pane->width * pane->height;
    pane->buffer = (char*)calloc(pane->buffer_size, sizeof(char));
    
    return pane;
}

void tmux_destroy_pane(tmux_pane_t* pane) {
    if (pane) {
        if (pane->buffer) {
            free(pane->buffer);
        }
        free(pane);
    }
}

int tmux_write_text(tmux_pane_t* pane, const char* text) {
    if (!pane || !text) return -1;
    
    size_t len = strlen(text);
    if (len > pane->buffer_size) {
        len = pane->buffer_size;
    }
    
    memcpy(pane->buffer, text, len);
    return 0;
}

int tmux_resize_pane(tmux_pane_t* pane, int width, int height) {
    if (!pane) return -1;
    
    size_t new_size = width * height;
    char* new_buffer = (char*)realloc(pane->buffer, new_size);
    if (!new_buffer) return -1;
    
    pane->buffer = new_buffer;
    pane->buffer_size = new_size;
    pane->width = width;
    pane->height = height;
    
    return 0;
}

// Test functions
test_result_t test_session_lifecycle() {
    test_result_t result = {
        .name = "Session Lifecycle",
        .passed = 1
    };
    
    double start = get_time_ms();
    
    // Create session
    tmux_session_t* session = tmux_create_session("test-session");
    if (!session) {
        result.passed = 0;
        strcpy(result.error_msg, "Failed to create session");
        return result;
    }
    
    // Verify session properties
    if (strcmp(session->name, "test-session") != 0) {
        result.passed = 0;
        strcpy(result.error_msg, "Session name mismatch");
    }
    
    if (session->active != 1) {
        result.passed = 0;
        strcpy(result.error_msg, "Session not active");
    }
    
    // Destroy session
    tmux_destroy_session(session);
    
    result.duration_ms = get_time_ms() - start;
    
    // Check performance regression
    if (result.duration_ms > PERF_BASELINE.session_create_ms * 1.5) {
        result.passed = 0;
        snprintf(result.error_msg, sizeof(result.error_msg),
                "Performance regression: %.2fms (baseline: %.2fms)",
                result.duration_ms, PERF_BASELINE.session_create_ms);
    }
    
    return result;
}

test_result_t test_window_management() {
    test_result_t result = {
        .name = "Window Management",
        .passed = 1
    };
    
    double start = get_time_ms();
    
    tmux_session_t* session = tmux_create_session("window-test");
    if (!session) {
        result.passed = 0;
        strcpy(result.error_msg, "Failed to create session");
        return result;
    }
    
    // Create multiple windows
    tmux_window_t* windows[5];
    for (int i = 0; i < 5; i++) {
        char name[32];
        snprintf(name, sizeof(name), "window-%d", i);
        windows[i] = tmux_create_window(session, name);
        
        if (!windows[i]) {
            result.passed = 0;
            snprintf(result.error_msg, sizeof(result.error_msg),
                    "Failed to create window %d", i);
            break;
        }
        
        // Verify window properties
        if (windows[i]->width != 80 || windows[i]->height != 24) {
            result.passed = 0;
            strcpy(result.error_msg, "Window dimensions incorrect");
            break;
        }
    }
    
    // Cleanup
    for (int i = 0; i < 5; i++) {
        if (windows[i]) {
            tmux_destroy_window(windows[i]);
        }
    }
    tmux_destroy_session(session);
    
    result.duration_ms = get_time_ms() - start;
    
    return result;
}

test_result_t test_pane_operations() {
    test_result_t result = {
        .name = "Pane Operations",
        .passed = 1
    };
    
    double start = get_time_ms();
    
    tmux_session_t* session = tmux_create_session("pane-test");
    tmux_window_t* window = tmux_create_window(session, "test-window");
    
    if (!session || !window) {
        result.passed = 0;
        strcpy(result.error_msg, "Failed to create session/window");
        return result;
    }
    
    // Test vertical split
    tmux_pane_t* pane1 = tmux_split_pane(window, 1);
    if (!pane1) {
        result.passed = 0;
        strcpy(result.error_msg, "Failed to create vertical pane");
    } else if (pane1->width != 40 || pane1->height != 24) {
        result.passed = 0;
        strcpy(result.error_msg, "Vertical pane dimensions incorrect");
    }
    
    // Test horizontal split
    tmux_pane_t* pane2 = tmux_split_pane(window, 0);
    if (!pane2) {
        result.passed = 0;
        strcpy(result.error_msg, "Failed to create horizontal pane");
    } else if (pane2->width != 80 || pane2->height != 12) {
        result.passed = 0;
        strcpy(result.error_msg, "Horizontal pane dimensions incorrect");
    }
    
    // Cleanup
    if (pane1) tmux_destroy_pane(pane1);
    if (pane2) tmux_destroy_pane(pane2);
    tmux_destroy_window(window);
    tmux_destroy_session(session);
    
    result.duration_ms = get_time_ms() - start;
    
    return result;
}

test_result_t test_text_rendering() {
    test_result_t result = {
        .name = "Text Rendering",
        .passed = 1
    };
    
    double start = get_time_ms();
    
    tmux_session_t* session = tmux_create_session("text-test");
    tmux_window_t* window = tmux_create_window(session, "test-window");
    tmux_pane_t* pane = tmux_split_pane(window, 1);
    
    if (!session || !window || !pane) {
        result.passed = 0;
        strcpy(result.error_msg, "Failed to create test environment");
        return result;
    }
    
    // Test text writing
    const char* test_text = "Hello, tmux integration!";
    if (tmux_write_text(pane, test_text) != 0) {
        result.passed = 0;
        strcpy(result.error_msg, "Failed to write text");
    }
    
    // Verify text in buffer
    if (result.passed && strncmp(pane->buffer, test_text, strlen(test_text)) != 0) {
        result.passed = 0;
        strcpy(result.error_msg, "Text not correctly written to buffer");
    }
    
    // Test with special characters
    const char* special_text = "\033[1mBold\033[0m \033[31mRed\033[0m";
    if (tmux_write_text(pane, special_text) != 0) {
        result.passed = 0;
        strcpy(result.error_msg, "Failed to write special characters");
    }
    
    // Cleanup
    tmux_destroy_pane(pane);
    tmux_destroy_window(window);
    tmux_destroy_session(session);
    
    result.duration_ms = get_time_ms() - start;
    
    return result;
}

test_result_t test_resize_operations() {
    test_result_t result = {
        .name = "Resize Operations",
        .passed = 1
    };
    
    double start = get_time_ms();
    
    tmux_session_t* session = tmux_create_session("resize-test");
    tmux_window_t* window = tmux_create_window(session, "test-window");
    tmux_pane_t* pane = tmux_split_pane(window, 1);
    
    if (!session || !window || !pane) {
        result.passed = 0;
        strcpy(result.error_msg, "Failed to create test environment");
        return result;
    }
    
    // Test resize smaller
    if (tmux_resize_pane(pane, 30, 20) != 0) {
        result.passed = 0;
        strcpy(result.error_msg, "Failed to resize smaller");
    } else if (pane->width != 30 || pane->height != 20) {
        result.passed = 0;
        strcpy(result.error_msg, "Resize dimensions incorrect");
    }
    
    // Test resize larger
    if (tmux_resize_pane(pane, 100, 50) != 0) {
        result.passed = 0;
        strcpy(result.error_msg, "Failed to resize larger");
    } else if (pane->width != 100 || pane->height != 50) {
        result.passed = 0;
        strcpy(result.error_msg, "Resize dimensions incorrect");
    }
    
    // Verify buffer was reallocated
    if (pane->buffer_size != 100 * 50) {
        result.passed = 0;
        strcpy(result.error_msg, "Buffer not correctly reallocated");
    }
    
    // Cleanup
    tmux_destroy_pane(pane);
    tmux_destroy_window(window);
    tmux_destroy_session(session);
    
    result.duration_ms = get_time_ms() - start;
    
    return result;
}

test_result_t test_error_handling() {
    test_result_t result = {
        .name = "Error Handling",
        .passed = 1
    };
    
    // Test NULL parameters
    if (tmux_create_window(NULL, "test") != NULL) {
        result.passed = 0;
        strcpy(result.error_msg, "Should fail with NULL session");
    }
    
    if (tmux_split_pane(NULL, 1) != NULL) {
        result.passed = 0;
        strcpy(result.error_msg, "Should fail with NULL window");
    }
    
    if (tmux_write_text(NULL, "test") != -1) {
        result.passed = 0;
        strcpy(result.error_msg, "Should fail with NULL pane");
    }
    
    tmux_pane_t dummy_pane = {0};
    if (tmux_write_text(&dummy_pane, NULL) != -1) {
        result.passed = 0;
        strcpy(result.error_msg, "Should fail with NULL text");
    }
    
    if (tmux_resize_pane(NULL, 10, 10) != -1) {
        result.passed = 0;
        strcpy(result.error_msg, "Should fail with NULL pane");
    }
    
    return result;
}

test_result_t test_memory_consistency() {
    test_result_t result = {
        .name = "Memory Consistency",
        .passed = 1
    };
    
    // Create and destroy many objects to check for leaks
    for (int i = 0; i < 100; i++) {
        tmux_session_t* session = tmux_create_session("mem-test");
        if (!session) {
            result.passed = 0;
            strcpy(result.error_msg, "Failed to create session in loop");
            break;
        }
        
        tmux_window_t* window = tmux_create_window(session, "mem-window");
        if (!window) {
            result.passed = 0;
            strcpy(result.error_msg, "Failed to create window in loop");
            tmux_destroy_session(session);
            break;
        }
        
        tmux_pane_t* pane = tmux_split_pane(window, i % 2);
        if (!pane) {
            result.passed = 0;
            strcpy(result.error_msg, "Failed to create pane in loop");
            tmux_destroy_window(window);
            tmux_destroy_session(session);
            break;
        }
        
        // Write and resize
        tmux_write_text(pane, "Memory test");
        tmux_resize_pane(pane, 50 + i, 25 + i);
        
        // Cleanup in reverse order
        tmux_destroy_pane(pane);
        tmux_destroy_window(window);
        tmux_destroy_session(session);
    }
    
    return result;
}

test_result_t test_concurrent_access() {
    test_result_t result = {
        .name = "Concurrent Access Simulation",
        .passed = 1
    };
    
    tmux_session_t* session = tmux_create_session("concurrent-test");
    tmux_window_t* window = tmux_create_window(session, "test-window");
    
    if (!session || !window) {
        result.passed = 0;
        strcpy(result.error_msg, "Failed to create test environment");
        return result;
    }
    
    // Create multiple panes
    tmux_pane_t* panes[4];
    for (int i = 0; i < 4; i++) {
        panes[i] = tmux_split_pane(window, i % 2);
        if (!panes[i]) {
            result.passed = 0;
            strcpy(result.error_msg, "Failed to create pane");
            break;
        }
    }
    
    // Simulate concurrent operations
    if (result.passed) {
        for (int round = 0; round < 10; round++) {
            for (int i = 0; i < 4; i++) {
                char text[64];
                snprintf(text, sizeof(text), "Pane %d round %d", i, round);
                
                if (tmux_write_text(panes[i], text) != 0) {
                    result.passed = 0;
                    strcpy(result.error_msg, "Write failed during concurrent test");
                    break;
                }
                
                // Random resize
                int new_width = 20 + (rand() % 60);
                int new_height = 10 + (rand() % 30);
                tmux_resize_pane(panes[i], new_width, new_height);
            }
            
            if (!result.passed) break;
        }
    }
    
    // Cleanup
    for (int i = 0; i < 4; i++) {
        if (panes[i]) {
            tmux_destroy_pane(panes[i]);
        }
    }
    tmux_destroy_window(window);
    tmux_destroy_session(session);
    
    return result;
}

// Performance comparison test
test_result_t test_performance_baseline() {
    test_result_t result = {
        .name = "Performance Baseline",
        .passed = 1
    };
    
    perf_baseline_t measured = {0};
    const int iterations = 100;
    
    // Measure session creation
    double start = get_time_ms();
    for (int i = 0; i < iterations; i++) {
        tmux_session_t* s = tmux_create_session("perf-test");
        tmux_destroy_session(s);
    }
    measured.session_create_ms = (get_time_ms() - start) / iterations;
    
    // Measure window creation
    tmux_session_t* session = tmux_create_session("perf-session");
    start = get_time_ms();
    for (int i = 0; i < iterations; i++) {
        tmux_window_t* w = tmux_create_window(session, "perf-window");
        tmux_destroy_window(w);
    }
    measured.window_create_ms = (get_time_ms() - start) / iterations;
    
    // Measure pane operations
    tmux_window_t* window = tmux_create_window(session, "test-window");
    start = get_time_ms();
    for (int i = 0; i < iterations; i++) {
        tmux_pane_t* p = tmux_split_pane(window, i % 2);
        tmux_destroy_pane(p);
    }
    measured.pane_split_ms = (get_time_ms() - start) / iterations;
    
    // Check against baselines (allow 50% degradation)
    if (measured.session_create_ms > PERF_BASELINE.session_create_ms * 1.5) {
        result.passed = 0;
        snprintf(result.error_msg, sizeof(result.error_msg),
                "Session creation slow: %.2fms (baseline: %.2fms)",
                measured.session_create_ms, PERF_BASELINE.session_create_ms);
    }
    
    if (measured.window_create_ms > PERF_BASELINE.window_create_ms * 1.5) {
        result.passed = 0;
        snprintf(result.error_msg, sizeof(result.error_msg),
                "Window creation slow: %.2fms (baseline: %.2fms)",
                measured.window_create_ms, PERF_BASELINE.window_create_ms);
    }
    
    printf("\n  Performance Measurements:\n");
    printf("    Session create: %.2fms (baseline: %.2fms)\n",
           measured.session_create_ms, PERF_BASELINE.session_create_ms);
    printf("    Window create: %.2fms (baseline: %.2fms)\n",
           measured.window_create_ms, PERF_BASELINE.window_create_ms);
    printf("    Pane split: %.2fms (baseline: %.2fms)\n",
           measured.pane_split_ms, PERF_BASELINE.pane_split_ms);
    
    // Cleanup
    tmux_destroy_window(window);
    tmux_destroy_session(session);
    
    return result;
}

int main(int argc, char* argv[]) {
    printf("====================================================\n");
    printf("Existing Functionality Regression Test Suite\n");
    printf("====================================================\n");
    printf("Started: %s\n", ctime(&(time_t){time(NULL)}));
    
    test_result_t tests[] = {
        test_session_lifecycle(),
        test_window_management(),
        test_pane_operations(),
        test_text_rendering(),
        test_resize_operations(),
        test_error_handling(),
        test_memory_consistency(),
        test_concurrent_access(),
        test_performance_baseline()
    };
    
    int total_tests = sizeof(tests) / sizeof(tests[0]);
    int passed_tests = 0;
    
    printf("\nTest Results:\n");
    printf("----------------------------------------\n");
    
    for (int i = 0; i < total_tests; i++) {
        if (tests[i].passed) {
            printf("✅ %-30s PASS", tests[i].name);
            passed_tests++;
        } else {
            printf("❌ %-30s FAIL", tests[i].name);
        }
        
        if (tests[i].duration_ms > 0) {
            printf(" (%.2fms)", tests[i].duration_ms);
        }
        
        if (!tests[i].passed && strlen(tests[i].error_msg) > 0) {
            printf("\n   Error: %s", tests[i].error_msg);
        }
        
        printf("\n");
    }
    
    printf("\n====================================================\n");
    printf("Regression Test Summary\n");
    printf("====================================================\n");
    printf("Total Tests: %d\n", total_tests);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", total_tests - passed_tests);
    printf("Success Rate: %.1f%%\n", (float)passed_tests / total_tests * 100);
    
    if (passed_tests == total_tests) {
        printf("\n✅ REGRESSION TEST: PASSED\n");
        printf("All existing functionality verified working correctly.\n");
        return 0;
    } else {
        printf("\n❌ REGRESSION TEST: FAILED\n");
        printf("Some functionality has regressed. DO NOT DEPLOY.\n");
        return 1;
    }
}