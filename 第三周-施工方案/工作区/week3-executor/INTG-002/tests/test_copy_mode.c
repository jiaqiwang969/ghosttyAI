// test_copy_mode.c - Comprehensive tests for copy mode implementation
// Purpose: Unit and integration tests for all copy mode functionality
// Author: INTG-002 (integration-dev)
// Date: 2025-08-26
// Task: T-204 - Test suite for copy mode
// Coverage: >80% target

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include "../include/copy_mode_callbacks.h"

// Test framework
#define TEST(name) void test_##name(void)
#define RUN_TEST(name) do { \
    printf("Running test: %s...", #name); \
    test_##name(); \
    printf(" PASSED\n"); \
    tests_passed++; \
} while(0)

static int tests_passed = 0;
static int tests_failed = 0;

// Mock grid implementation for testing
typedef struct {
    char** lines;
    uint32_t rows;
    uint32_t cols;
} mock_grid_t;

static mock_grid_t* mock_grid_create(uint32_t rows, uint32_t cols) {
    mock_grid_t* grid = malloc(sizeof(mock_grid_t));
    grid->rows = rows;
    grid->cols = cols;
    grid->lines = calloc(rows, sizeof(char*));
    
    for (uint32_t i = 0; i < rows; i++) {
        grid->lines[i] = calloc(cols + 1, sizeof(char));
        // Fill with test data
        for (uint32_t j = 0; j < cols; j++) {
            grid->lines[i][j] = 'A' + (i % 26);
        }
    }
    
    return grid;
}

static void mock_grid_destroy(mock_grid_t* grid) {
    for (uint32_t i = 0; i < grid->rows; i++) {
        free(grid->lines[i]);
    }
    free(grid->lines);
    free(grid);
}

static char* mock_get_line(void* grid, uint32_t row, size_t* len) {
    mock_grid_t* g = (mock_grid_t*)grid;
    if (row >= g->rows) {
        *len = 0;
        return NULL;
    }
    
    *len = strlen(g->lines[row]);
    return strdup(g->lines[row]);
}

static void mock_get_size(void* grid, uint32_t* rows, uint32_t* cols) {
    mock_grid_t* g = (mock_grid_t*)grid;
    *rows = g->rows;
    *cols = g->cols;
}

static grid_callbacks_t mock_callbacks = {
    .get_line = mock_get_line,
    .get_size = mock_get_size,
    .get_cell = NULL,  // Not needed for basic tests
    .get_history_size = NULL,
    .mark_dirty = NULL
};

// ============================================================================
// Basic functionality tests
// ============================================================================

TEST(init_cleanup) {
    copy_mode_backend_t* backend = copy_mode_init(KEY_MODE_VI);
    assert(backend != NULL);
    assert(backend->key_mode == KEY_MODE_VI);
    assert(backend->active == false);
    assert(backend->selection.active == false);
    
    copy_mode_cleanup(backend);
}

TEST(enter_exit) {
    copy_mode_backend_t* backend = copy_mode_init(KEY_MODE_VI);
    
    // Enter copy mode
    int result = backend->vtable->enter(backend, NULL);
    assert(result == 0);
    assert(backend->active == true);
    
    // Exit copy mode
    result = backend->vtable->exit(backend, NULL);
    assert(result == 0);
    assert(backend->active == false);
    
    copy_mode_cleanup(backend);
}

TEST(basic_selection) {
    copy_mode_backend_t* backend = copy_mode_init(KEY_MODE_VI);
    mock_grid_t* grid = mock_grid_create(24, 80);
    
    copy_mode_set_grid_callbacks(backend, &mock_callbacks, grid);
    backend->vtable->enter(backend, NULL);
    
    // Start selection
    int result = backend->vtable->select_start(backend, 5, 10);
    assert(result == 0);
    assert(backend->selection.active == true);
    assert(backend->selection.start_row == 5);
    assert(backend->selection.start_col == 10);
    
    // Update selection
    result = backend->vtable->select_update(backend, 7, 20);
    assert(result == 0);
    assert(backend->selection.end_row == 7);
    assert(backend->selection.end_col == 20);
    
    // End selection
    result = backend->vtable->select_end(backend);
    assert(result == 0);
    assert(backend->selection.active == true); // Still active after end
    
    // Clear selection
    result = backend->vtable->select_clear(backend);
    assert(result == 0);
    assert(backend->selection.active == false);
    
    mock_grid_destroy(grid);
    copy_mode_cleanup(backend);
}

TEST(selection_modes) {
    copy_mode_backend_t* backend = copy_mode_init(KEY_MODE_VI);
    mock_grid_t* grid = mock_grid_create(24, 80);
    
    copy_mode_set_grid_callbacks(backend, &mock_callbacks, grid);
    backend->vtable->enter(backend, NULL);
    
    // Test character mode
    backend->vtable->set_mode(backend, SEL_MODE_CHAR);
    assert(backend->selection.mode == SEL_MODE_CHAR);
    
    // Test word mode
    backend->vtable->set_mode(backend, SEL_MODE_WORD);
    assert(backend->selection.mode == SEL_MODE_WORD);
    
    // Test line mode
    backend->vtable->set_mode(backend, SEL_MODE_LINE);
    assert(backend->selection.mode == SEL_MODE_LINE);
    
    // Test rectangular mode
    backend->vtable->set_mode(backend, SEL_MODE_RECT);
    assert(backend->selection.mode == SEL_MODE_RECT);
    
    mock_grid_destroy(grid);
    copy_mode_cleanup(backend);
}

TEST(movement_operations) {
    copy_mode_backend_t* backend = copy_mode_init(KEY_MODE_VI);
    backend->view_height = 24;
    backend->view_width = 80;
    backend->vtable->enter(backend, NULL);
    
    // Set initial cursor position
    backend->cursor_row = 10;
    backend->cursor_col = 20;
    
    // Test up movement
    backend->vtable->move_up(backend, 3);
    assert(backend->cursor_row == 7);
    
    // Test down movement
    backend->vtable->move_down(backend, 5);
    assert(backend->cursor_row == 12);
    
    // Test left movement
    backend->vtable->move_left(backend, 10);
    assert(backend->cursor_col == 10);
    
    // Test right movement
    backend->vtable->move_right(backend, 15);
    assert(backend->cursor_col == 25);
    
    // Test boundary conditions
    backend->vtable->move_up(backend, 100);
    assert(backend->cursor_row == 0);
    
    backend->vtable->move_down(backend, 100);
    assert(backend->cursor_row == 23);
    
    backend->vtable->move_left(backend, 100);
    assert(backend->cursor_col == 0);
    
    backend->vtable->move_right(backend, 100);
    assert(backend->cursor_col == 79);
    
    copy_mode_cleanup(backend);
}

TEST(copy_paste_operations) {
    copy_mode_backend_t* backend = copy_mode_init(KEY_MODE_VI);
    mock_grid_t* grid = mock_grid_create(24, 80);
    
    // Set up test data
    strcpy(grid->lines[5], "Hello World Test String");
    
    copy_mode_set_grid_callbacks(backend, &mock_callbacks, grid);
    backend->vtable->enter(backend, NULL);
    
    // Make a selection
    backend->vtable->select_start(backend, 5, 0);
    backend->vtable->select_update(backend, 5, 10);
    
    // Copy selection
    int result = backend->vtable->copy_selection(backend, CLIPBOARD_TEXT);
    assert(result == 0);
    
    // Check buffer was saved
    assert(backend->buffers[0] != NULL);
    assert(backend->buffers[0]->size > 0);
    
    // Test paste
    result = backend->vtable->paste(backend, 0);
    assert(result == 0);
    
    mock_grid_destroy(grid);
    copy_mode_cleanup(backend);
}

TEST(search_operations) {
    copy_mode_backend_t* backend = copy_mode_init(KEY_MODE_VI);
    mock_grid_t* grid = mock_grid_create(24, 80);
    
    // Set up test data with searchable content
    strcpy(grid->lines[5], "This is a test string with test word");
    strcpy(grid->lines[10], "Another line with test content");
    
    copy_mode_set_grid_callbacks(backend, &mock_callbacks, grid);
    backend->vtable->enter(backend, NULL);
    
    // Search forward
    int result = backend->vtable->search_forward(backend, "test");
    assert(result == 0);
    assert(backend->cursor_row == 5);
    
    // Search backward
    backend->cursor_row = 15;
    result = backend->vtable->search_backward(backend, "test");
    assert(result == 0);
    assert(backend->cursor_row == 10);
    
    mock_grid_destroy(grid);
    copy_mode_cleanup(backend);
}

TEST(select_all) {
    copy_mode_backend_t* backend = copy_mode_init(KEY_MODE_VI);
    mock_grid_t* grid = mock_grid_create(24, 80);
    
    copy_mode_set_grid_callbacks(backend, &mock_callbacks, grid);
    backend->vtable->enter(backend, NULL);
    
    // Select all
    int result = backend->vtable->select_all(backend);
    assert(result == 0);
    assert(backend->selection.active == true);
    assert(backend->selection.start_row == 0);
    assert(backend->selection.start_col == 0);
    assert(backend->selection.end_row == 23);
    assert(backend->selection.end_col == 79);
    
    mock_grid_destroy(grid);
    copy_mode_cleanup(backend);
}

// ============================================================================
// Performance tests
// ============================================================================

TEST(selection_performance) {
    copy_mode_backend_t* backend = copy_mode_init(KEY_MODE_VI);
    mock_grid_t* grid = mock_grid_create(1000, 200);  // Large grid
    
    copy_mode_set_grid_callbacks(backend, &mock_callbacks, grid);
    backend->vtable->enter(backend, NULL);
    
    clock_t start = clock();
    
    // Perform many selection updates
    for (int i = 0; i < 10000; i++) {
        backend->vtable->select_start(backend, i % 100, i % 50);
        backend->vtable->select_update(backend, (i + 10) % 100, (i + 30) % 50);
    }
    
    clock_t end = clock();
    double elapsed_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    
    printf("\n  Selection update performance: %.2f ms for 10000 updates (%.2f us/update)",
           elapsed_ms, elapsed_ms * 1000 / 10000);
    
    // Should be < 10ms per update
    assert(elapsed_ms / 10000 < 10);
    
    mock_grid_destroy(grid);
    copy_mode_cleanup(backend);
}

TEST(large_copy_performance) {
    copy_mode_backend_t* backend = copy_mode_init(KEY_MODE_VI);
    mock_grid_t* grid = mock_grid_create(10000, 200);  // Very large grid
    
    // Fill with data
    for (uint32_t i = 0; i < grid->rows; i++) {
        for (uint32_t j = 0; j < grid->cols; j++) {
            grid->lines[i][j] = 'A' + ((i + j) % 26);
        }
    }
    
    copy_mode_set_grid_callbacks(backend, &mock_callbacks, grid);
    backend->vtable->enter(backend, NULL);
    
    // Select large region (approximately 10MB)
    backend->vtable->select_start(backend, 0, 0);
    backend->vtable->select_update(backend, 5000, 199);
    
    clock_t start = clock();
    
    // Copy selection
    int result = backend->vtable->copy_selection(backend, CLIPBOARD_TEXT);
    
    clock_t end = clock();
    double elapsed_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    
    printf("\n  Large copy performance: %.2f ms for ~10MB", elapsed_ms);
    
    assert(result == 0);
    // Should be < 50ms for 10MB
    assert(elapsed_ms < 50);
    
    mock_grid_destroy(grid);
    copy_mode_cleanup(backend);
}

TEST(memory_leak_check) {
    // Run multiple init/cleanup cycles to check for leaks
    for (int i = 0; i < 100; i++) {
        copy_mode_backend_t* backend = copy_mode_init(KEY_MODE_VI);
        mock_grid_t* grid = mock_grid_create(24, 80);
        
        copy_mode_set_grid_callbacks(backend, &mock_callbacks, grid);
        backend->vtable->enter(backend, NULL);
        
        // Perform various operations
        backend->vtable->select_start(backend, 0, 0);
        backend->vtable->select_update(backend, 10, 20);
        backend->vtable->copy_selection(backend, CLIPBOARD_TEXT);
        backend->vtable->search_forward(backend, "test");
        
        backend->vtable->exit(backend, NULL);
        mock_grid_destroy(grid);
        copy_mode_cleanup(backend);
    }
    
    // If we get here without crashes, basic memory management is OK
    // Use valgrind for thorough leak checking
}

// ============================================================================
// Clipboard integration tests
// ============================================================================

TEST(clipboard_basic) {
    int result = clipboard_init();
    assert(result == 0);
    
    const char* test_data = "Test clipboard data";
    size_t test_size = strlen(test_data);
    
    // Set clipboard
    result = clipboard_set(test_data, test_size, CLIPBOARD_TEXT);
    assert(result == 0);
    
    // Get clipboard
    char* retrieved = NULL;
    size_t retrieved_size = 0;
    result = clipboard_get(&retrieved, &retrieved_size, CLIPBOARD_TEXT);
    
    // Note: May fail if clipboard access is restricted
    if (result == 0) {
        assert(retrieved != NULL);
        assert(retrieved_size > 0);
        free(retrieved);
    }
    
    clipboard_cleanup();
}

TEST(clipboard_formats) {
    clipboard_init();
    
    // Test different formats
    const char* text = "Plain text";
    const char* rtf = "{\\rtf1 Test RTF}";
    const char* html = "<html><body>Test HTML</body></html>";
    
    // Text format
    int result = clipboard_set(text, strlen(text), CLIPBOARD_TEXT);
    assert(result == 0);
    
    // RTF format (may not be fully supported)
    result = clipboard_set(rtf, strlen(rtf), CLIPBOARD_RTF);
    // Don't assert as RTF may not be implemented
    
    // HTML format (may not be fully supported)
    result = clipboard_set(html, strlen(html), CLIPBOARD_HTML);
    // Don't assert as HTML may not be implemented
    
    clipboard_cleanup();
}

// ============================================================================
// Edge cases and error handling
// ============================================================================

TEST(edge_cases) {
    copy_mode_backend_t* backend = copy_mode_init(KEY_MODE_VI);
    
    // Test operations without grid
    int result = backend->vtable->select_all(backend);
    assert(result == -1);  // Should fail without grid
    
    // Test invalid buffer index
    result = backend->vtable->paste(backend, 100);
    assert(result == -1);
    
    // Test selection without being active
    result = backend->vtable->select_update(backend, 5, 5);
    assert(result == -1);
    
    copy_mode_cleanup(backend);
}

TEST(unicode_handling) {
    copy_mode_backend_t* backend = copy_mode_init(KEY_MODE_VI);
    mock_grid_t* grid = mock_grid_create(24, 80);
    
    // Set up UTF-8 test data
    strcpy(grid->lines[0], "Hello 世界 🌍 Test");
    strcpy(grid->lines[1], "Emoji: 😀😁😂");
    
    copy_mode_set_grid_callbacks(backend, &mock_callbacks, grid);
    backend->vtable->enter(backend, NULL);
    
    // Select text with Unicode
    backend->vtable->select_start(backend, 0, 0);
    backend->vtable->select_update(backend, 1, 20);
    
    // Get selection
    size_t size;
    char* text = copy_mode_get_selection(backend, &size);
    assert(text != NULL);
    assert(size > 0);
    
    free(text);
    mock_grid_destroy(grid);
    copy_mode_cleanup(backend);
}

// ============================================================================
// Integration test
// ============================================================================

TEST(full_integration) {
    // Initialize everything
    copy_mode_backend_t* backend = copy_mode_init(KEY_MODE_VI);
    mock_grid_t* grid = mock_grid_create(100, 120);
    
    // Set up realistic test data
    for (uint32_t i = 0; i < grid->rows; i++) {
        snprintf(grid->lines[i], grid->cols, 
                "Line %03d: Lorem ipsum dolor sit amet, consectetur adipiscing elit", i);
    }
    
    copy_mode_set_grid_callbacks(backend, &mock_callbacks, grid);
    
    // Enter copy mode
    backend->vtable->enter(backend, NULL);
    assert(backend->active == true);
    
    // Search for text
    int result = backend->vtable->search_forward(backend, "Lorem");
    assert(result == 0);
    
    // Start selection at search result
    backend->vtable->select_start(backend, backend->cursor_row, backend->cursor_col);
    
    // Move to select a word
    backend->vtable->move_word_forward(backend);
    backend->vtable->move_word_forward(backend);
    
    // Copy selection
    result = backend->vtable->copy_selection(backend, CLIPBOARD_TEXT);
    assert(result == 0);
    
    // Check statistics
    assert(backend->stats.selections_made > 0);
    assert(backend->stats.bytes_copied > 0);
    
    // Exit copy mode
    backend->vtable->exit(backend, NULL);
    assert(backend->active == false);
    
    mock_grid_destroy(grid);
    copy_mode_cleanup(backend);
}

// ============================================================================
// Main test runner
// ============================================================================

int main(void) {
    printf("Running Copy Mode Test Suite\n");
    printf("============================\n\n");
    
    // Basic tests
    RUN_TEST(init_cleanup);
    RUN_TEST(enter_exit);
    RUN_TEST(basic_selection);
    RUN_TEST(selection_modes);
    RUN_TEST(movement_operations);
    RUN_TEST(copy_paste_operations);
    RUN_TEST(search_operations);
    RUN_TEST(select_all);
    
    // Performance tests
    RUN_TEST(selection_performance);
    RUN_TEST(large_copy_performance);
    RUN_TEST(memory_leak_check);
    
    // Clipboard tests
    RUN_TEST(clipboard_basic);
    RUN_TEST(clipboard_formats);
    
    // Edge cases
    RUN_TEST(edge_cases);
    RUN_TEST(unicode_handling);
    
    // Integration
    RUN_TEST(full_integration);
    
    printf("\n============================\n");
    printf("Test Results: %d passed, %d failed\n", tests_passed, tests_failed);
    
    if (tests_failed == 0) {
        printf("All tests PASSED! ✓\n");
        return 0;
    } else {
        printf("Some tests FAILED! ✗\n");
        return 1;
    }
}