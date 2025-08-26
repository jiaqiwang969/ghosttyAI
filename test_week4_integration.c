// test_week4_integration.c - Week 4 Integration Test Program
// Purpose: Verify the complete callback chain from tmux to UI Backend
// Date: 2025-08-26
// Task: W4-INC-005 - End-to-end testing

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Include minimal tmux headers
#include "tmux/ui_backend/ui_backend_minimal.h"

// External functions we need to declare
extern void ui_backend_set_callbacks(void* callbacks, void* user_data);
extern int ui_backend_has_callbacks(void);
extern const char* ui_backend_get_status(void);
extern void ui_backend_flush(void);

// Callback function prototypes matching ui_callbacks_t
typedef struct {
    void (*on_cell)(char ch, int row, int col, int attr, int fg, int bg, void* user_data);
    void (*on_clear_line)(int row, void* user_data);
    void (*on_clear_screen)(void* user_data);
    void (*on_insert_line)(int row, void* user_data);
    void (*on_delete_line)(int row, void* user_data);
    void (*on_clear_eol)(int row, int col, void* user_data);
    void (*on_reverse_index)(void* user_data);
    void (*on_line_feed)(void* user_data);
    void (*on_scroll_up)(int count, void* user_data);
    void (*on_scroll_down)(int count, void* user_data);
    void (*on_flush)(void* user_data);
} ui_callbacks_t;

// Test grid storage
#define GRID_ROWS 24
#define GRID_COLS 80

typedef struct {
    char grid[GRID_ROWS][GRID_COLS];
    int cursor_row;
    int cursor_col;
} test_context_t;

// Test callback implementations
void test_on_cell(char ch, int row, int col, int attr, int fg, int bg, void* user_data) {
    printf("[TEST CALLBACK] Cell '%c' at (%d,%d) - attr=%d fg=0x%06X bg=0x%06X\n", 
           ch, row, col, attr, fg, bg);
    
    if (user_data) {
        test_context_t* ctx = (test_context_t*)user_data;
        if (row >= 0 && row < GRID_ROWS && col >= 0 && col < GRID_COLS) {
            ctx->grid[row][col] = ch;
            ctx->cursor_row = row;
            ctx->cursor_col = col + 1;
        }
    }
}

void test_on_clear_line(int row, void* user_data) {
    printf("[TEST CALLBACK] Clear line %d\n", row);
    
    if (user_data) {
        test_context_t* ctx = (test_context_t*)user_data;
        if (row >= 0 && row < GRID_ROWS) {
            memset(ctx->grid[row], ' ', GRID_COLS);
        }
    }
}

void test_on_clear_screen(void* user_data) {
    printf("[TEST CALLBACK] Clear screen\n");
    
    if (user_data) {
        test_context_t* ctx = (test_context_t*)user_data;
        for (int i = 0; i < GRID_ROWS; i++) {
            memset(ctx->grid[i], ' ', GRID_COLS);
        }
        ctx->cursor_row = 0;
        ctx->cursor_col = 0;
    }
}

void test_on_flush(void* user_data) {
    printf("[TEST CALLBACK] Flush requested\n");
    
    if (user_data) {
        test_context_t* ctx = (test_context_t*)user_data;
        printf("\n=== Grid Display ===\n");
        for (int r = 0; r < 10; r++) {  // Show first 10 rows
            printf("%2d: ", r);
            for (int c = 0; c < 40; c++) {  // Show first 40 columns
                char ch = ctx->grid[r][c];
                if (ch >= 0x20 && ch <= 0x7E) {
                    printf("%c", ch);
                } else {
                    printf(".");
                }
            }
            printf("\n");
        }
        printf("=== Cursor at (%d,%d) ===\n", ctx->cursor_row, ctx->cursor_col);
    }
}

// Function pointer declarations for tmux commands
void (*tty_cmd_cell_ptr)(struct tty *, const struct tty_ctx *) = NULL;
void (*tty_cmd_clearline_ptr)(struct tty *, const struct tty_ctx *) = NULL;

// Stub implementations of tty_cmd functions for testing
void tty_cmd_cell(struct tty* tty, const struct tty_ctx* ctx) {
    (void)tty;  // Unused
    (void)ctx;  // Unused
    printf("[TEST] tty_cmd_cell stub called\n");
}

void tty_cmd_clearline(struct tty* tty, const struct tty_ctx* ctx) {
    (void)tty;  // Unused
    (void)ctx;  // Unused
    printf("[TEST] tty_cmd_clearline stub called\n");
}

// Display test results
void display_test_results(int passed, int failed) {
    printf("\n");
    printf("=====================================\n");
    if (failed == 0) {
        printf("✅ ALL TESTS PASSED (%d/%d)\n", passed, passed + failed);
    } else {
        printf("❌ SOME TESTS FAILED\n");
        printf("   Passed: %d\n", passed);
        printf("   Failed: %d\n", failed);
    }
    printf("=====================================\n");
}

// Test functions
int test_ui_backend_initialization() {
    printf("\n[TEST 1] UI Backend Initialization\n");
    printf("---------------------------------------\n");
    
    // Set environment variable
    setenv("TMUX_UI_BACKEND", "ghostty", 1);
    
    // Initialize UI Backend
    int result = ui_backend_init();
    if (result != 0) {
        printf("❌ ui_backend_init() failed with code %d\n", result);
        return 0;
    }
    
    // Check if enabled
    if (!ui_backend_enabled()) {
        printf("❌ UI Backend not enabled after init\n");
        return 0;
    }
    
    printf("✅ UI Backend initialized successfully\n");
    return 1;
}

int test_callback_registration() {
    printf("\n[TEST 2] Callback Registration\n");
    printf("---------------------------------------\n");
    
    test_context_t test_ctx;
    memset(&test_ctx, 0, sizeof(test_ctx));
    
    // Initialize grid with spaces
    for (int i = 0; i < GRID_ROWS; i++) {
        memset(test_ctx.grid[i], ' ', GRID_COLS);
    }
    
    // Create callback structure
    ui_callbacks_t callbacks = {
        .on_cell = test_on_cell,
        .on_clear_line = test_on_clear_line,
        .on_clear_screen = test_on_clear_screen,
        .on_insert_line = NULL,
        .on_delete_line = NULL,
        .on_clear_eol = NULL,
        .on_reverse_index = NULL,
        .on_line_feed = NULL,
        .on_scroll_up = NULL,
        .on_scroll_down = NULL,
        .on_flush = test_on_flush
    };
    
    // Register callbacks
    ui_backend_set_callbacks(&callbacks, &test_ctx);
    
    // Check registration
    if (!ui_backend_has_callbacks()) {
        printf("❌ Callbacks not registered\n");
        return 0;
    }
    
    const char* status = ui_backend_get_status();
    printf("✅ Callbacks registered - Status: %s\n", status);
    return 1;
}

int test_cell_dispatch() {
    printf("\n[TEST 3] Cell Dispatch\n");
    printf("---------------------------------------\n");
    
    test_context_t test_ctx;
    memset(&test_ctx, 0, sizeof(test_ctx));
    
    // Initialize grid
    for (int i = 0; i < GRID_ROWS; i++) {
        memset(test_ctx.grid[i], ' ', GRID_COLS);
    }
    
    // Register callbacks
    ui_callbacks_t callbacks = {
        .on_cell = test_on_cell,
        .on_clear_line = test_on_clear_line,
        .on_clear_screen = test_on_clear_screen,
        .on_insert_line = NULL,
        .on_delete_line = NULL,
        .on_clear_eol = NULL,
        .on_reverse_index = NULL,
        .on_line_feed = NULL,
        .on_scroll_up = NULL,
        .on_scroll_down = NULL,
        .on_flush = test_on_flush
    };
    ui_backend_set_callbacks(&callbacks, &test_ctx);
    
    // Create test cell data
    struct grid_cell cell = {
        .data = { .size = 1, .data = "H" },
        .attr = 0,
        .fg = 0xFFFFFF,
        .bg = 0x000000
    };
    
    struct tty_ctx ctx = {
        .cell = &cell,
        .ocy = 0,
        .ocx = 0
    };
    
    // Get backend instance
    ui_backend_t* backend = ui_backend_get_instance();
    if (!backend) {
        printf("❌ Could not get backend instance\n");
        return 0;
    }
    
    // Store function pointer for testing
    tty_cmd_cell_ptr = tty_cmd_cell;
    
    // Test dispatch
    int result = ui_backend_dispatch(backend, tty_cmd_cell, &ctx);
    
    if (result != 0) {
        printf("❌ Dispatch failed with code %d\n", result);
        return 0;
    }
    
    // Verify cell was set
    if (test_ctx.grid[0][0] != 'H') {
        printf("❌ Cell not set in grid (expected 'H', got '%c')\n", test_ctx.grid[0][0]);
        return 0;
    }
    
    printf("✅ Cell 'H' successfully dispatched and stored\n");
    return 1;
}

int test_multiple_cells() {
    printf("\n[TEST 4] Multiple Cell Dispatch\n");
    printf("---------------------------------------\n");
    
    test_context_t test_ctx;
    memset(&test_ctx, 0, sizeof(test_ctx));
    
    // Initialize grid
    for (int i = 0; i < GRID_ROWS; i++) {
        memset(test_ctx.grid[i], ' ', GRID_COLS);
    }
    
    // Register callbacks
    ui_callbacks_t callbacks = {
        .on_cell = test_on_cell,
        .on_clear_line = test_on_clear_line,
        .on_clear_screen = test_on_clear_screen,
        .on_insert_line = NULL,
        .on_delete_line = NULL,
        .on_clear_eol = NULL,
        .on_reverse_index = NULL,
        .on_line_feed = NULL,
        .on_scroll_up = NULL,
        .on_scroll_down = NULL,
        .on_flush = test_on_flush
    };
    ui_backend_set_callbacks(&callbacks, &test_ctx);
    
    ui_backend_t* backend = ui_backend_get_instance();
    const char* test_string = "Hello, World!";
    
    // Send each character
    for (int i = 0; test_string[i]; i++) {
        struct grid_cell cell = {
            .data = { .size = 1, .data = {test_string[i]} },
            .attr = 0,
            .fg = 0xFFFFFF,
            .bg = 0x000000
        };
        
        struct tty_ctx ctx = {
            .cell = &cell,
            .ocy = 0,
            .ocx = i
        };
        
        int result = ui_backend_dispatch(backend, tty_cmd_cell, &ctx);
        if (result != 0) {
            printf("❌ Failed to dispatch character '%c' at position %d\n", test_string[i], i);
            return 0;
        }
    }
    
    // Verify the string was written
    char buffer[100] = {0};
    for (int i = 0; i < strlen(test_string); i++) {
        buffer[i] = test_ctx.grid[0][i];
    }
    
    if (strcmp(buffer, test_string) != 0) {
        printf("❌ String mismatch: expected '%s', got '%s'\n", test_string, buffer);
        return 0;
    }
    
    printf("✅ Successfully dispatched string: '%s'\n", test_string);
    
    // Request flush to display
    ui_backend_flush();
    
    return 1;
}

int test_clear_operations() {
    printf("\n[TEST 5] Clear Operations\n");
    printf("---------------------------------------\n");
    
    test_context_t test_ctx;
    memset(&test_ctx, 0, sizeof(test_ctx));
    
    // Initialize grid with 'X'
    for (int i = 0; i < GRID_ROWS; i++) {
        memset(test_ctx.grid[i], 'X', GRID_COLS);
    }
    
    // Register callbacks
    ui_callbacks_t callbacks = {
        .on_cell = test_on_cell,
        .on_clear_line = test_on_clear_line,
        .on_clear_screen = test_on_clear_screen,
        .on_insert_line = NULL,
        .on_delete_line = NULL,
        .on_clear_eol = NULL,
        .on_reverse_index = NULL,
        .on_line_feed = NULL,
        .on_scroll_up = NULL,
        .on_scroll_down = NULL,
        .on_flush = test_on_flush
    };
    ui_backend_set_callbacks(&callbacks, &test_ctx);
    
    ui_backend_t* backend = ui_backend_get_instance();
    
    // Test clear line
    struct tty_ctx ctx = {
        .ocy = 5,
        .ocx = 0
    };
    
    tty_cmd_clearline_ptr = tty_cmd_clearline;
    int result = ui_backend_dispatch(backend, tty_cmd_clearline, &ctx);
    
    if (result != 0) {
        printf("❌ Clear line dispatch failed\n");
        return 0;
    }
    
    // Verify line 5 was cleared
    int cleared = 1;
    for (int i = 0; i < GRID_COLS; i++) {
        if (test_ctx.grid[5][i] != ' ') {
            cleared = 0;
            break;
        }
    }
    
    if (!cleared) {
        printf("❌ Line 5 was not cleared properly\n");
        return 0;
    }
    
    printf("✅ Clear line operation successful\n");
    return 1;
}

int main(int argc, char* argv[]) {
    (void)argc;  // Unused
    (void)argv;  // Unused
    
    printf("=====================================\n");
    printf("    Week 4 Integration Test Suite    \n");
    printf("=====================================\n");
    
    int passed = 0;
    int failed = 0;
    
    // Run tests
    if (test_ui_backend_initialization()) {
        passed++;
    } else {
        failed++;
    }
    
    if (test_callback_registration()) {
        passed++;
    } else {
        failed++;
    }
    
    if (test_cell_dispatch()) {
        passed++;
    } else {
        failed++;
    }
    
    if (test_multiple_cells()) {
        passed++;
    } else {
        failed++;
    }
    
    if (test_clear_operations()) {
        passed++;
    } else {
        failed++;
    }
    
    // Display results
    display_test_results(passed, failed);
    
    // Cleanup
    ui_backend_cleanup();
    
    return (failed == 0) ? 0 : 1;
}