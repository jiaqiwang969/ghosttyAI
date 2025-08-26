// test_minimal_dispatch.c - Minimal test using command IDs
// Purpose: Test Week 4 dispatch with explicit command IDs
// Date: 2025-08-26

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tmux/ui_backend/ui_backend_minimal.h"

// Command IDs matching dispatch implementation
#define TTY_CMD_CELL 1
#define TTY_CMD_CLEARLINE 2
#define TTY_CMD_CLEARSCREEN 3

// External functions we need
extern void ui_backend_set_callbacks(void* callbacks, void* user_data);
extern int ui_backend_has_callbacks(void);
extern const char* ui_backend_get_status(void);
extern void ui_backend_flush(void);
extern int ui_backend_init(void);
extern ui_backend_t* ui_backend_get_instance(void);
extern int ui_backend_dispatch(ui_backend_t* backend, 
                              void (*cmdfn)(struct tty *, const struct tty_ctx *),
                              struct tty_ctx* ctx);

// Callback function prototypes
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

// Simple test context
typedef struct {
    char last_char;
    int last_row;
    int last_col;
    int cell_count;
    int clear_count;
} test_context_t;

// Test callbacks
void test_on_cell(char ch, int row, int col, int attr, int fg, int bg, void* user_data) {
    test_context_t* ctx = (test_context_t*)user_data;
    printf("[CALLBACK] Cell '%c' at (%d,%d)\n", ch, row, col);
    if (ctx) {
        ctx->last_char = ch;
        ctx->last_row = row;
        ctx->last_col = col;
        ctx->cell_count++;
    }
}

void test_on_clear_line(int row, void* user_data) {
    test_context_t* ctx = (test_context_t*)user_data;
    printf("[CALLBACK] Clear line %d\n", row);
    if (ctx) {
        ctx->clear_count++;
    }
}

void test_on_clear_screen(void* user_data) {
    test_context_t* ctx = (test_context_t*)user_data;
    printf("[CALLBACK] Clear screen\n");
    if (ctx) {
        ctx->clear_count++;
    }
}

// Dummy tty_cmd function (won't be called, just for function pointer)
void dummy_tty_cmd(struct tty* tty, const struct tty_ctx* ctx) {
    (void)tty;
    (void)ctx;
}

int main() {
    printf("\n=== Minimal Dispatch Test ===\n\n");
    
    // Initialize UI Backend
    printf("1. Initializing UI Backend...\n");
    setenv("TMUX_UI_BACKEND", "ghostty", 1);
    if (ui_backend_init() != 0) {
        printf("❌ Failed to initialize UI Backend\n");
        return 1;
    }
    printf("✅ UI Backend initialized\n\n");
    
    // Set up callbacks
    printf("2. Setting up callbacks...\n");
    test_context_t test_ctx = {0};
    
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
        .on_flush = NULL
    };
    
    ui_backend_set_callbacks(&callbacks, &test_ctx);
    
    if (!ui_backend_has_callbacks()) {
        printf("❌ Callbacks not registered\n");
        return 1;
    }
    printf("✅ Callbacks registered: %s\n\n", ui_backend_get_status());
    
    // Get backend instance
    ui_backend_t* backend = ui_backend_get_instance();
    if (!backend) {
        printf("❌ Could not get backend instance\n");
        return 1;
    }
    
    // Test 1: Cell with explicit command ID
    printf("3. Testing cell dispatch with command ID...\n");
    
    struct grid_cell cell = {0};
    cell.data.size = 1;
    cell.data.data[0] = 'H';
    cell.fg = 0xFFFFFF;
    cell.bg = 0x000000;
    
    struct tty_ctx ctx = {0};
    ctx.cell = &cell;
    ctx.ocy = 0;
    ctx.ocx = 0;
    ctx.ui_cmd_id = TTY_CMD_CELL;  // Set explicit command ID
    
    int result = ui_backend_dispatch(backend, dummy_tty_cmd, &ctx);
    
    if (result == 0 && test_ctx.last_char == 'H') {
        printf("✅ Cell dispatch successful: received '%c' at (%d,%d)\n", 
               test_ctx.last_char, test_ctx.last_row, test_ctx.last_col);
    } else {
        printf("❌ Cell dispatch failed (result=%d, char='%c')\n", 
               result, test_ctx.last_char);
    }
    
    // Test 2: Clear line with explicit command ID
    printf("\n4. Testing clear line dispatch with command ID...\n");
    
    ctx.ui_cmd_id = TTY_CMD_CLEARLINE;  // Set explicit command ID
    ctx.ocy = 5;
    ctx.cell = NULL;
    
    result = ui_backend_dispatch(backend, dummy_tty_cmd, &ctx);
    
    if (result == 0 && test_ctx.clear_count > 0) {
        printf("✅ Clear line dispatch successful\n");
    } else {
        printf("❌ Clear line dispatch failed (result=%d)\n", result);
    }
    
    // Summary
    printf("\n=== Test Summary ===\n");
    printf("Cells processed: %d\n", test_ctx.cell_count);
    printf("Clears processed: %d\n", test_ctx.clear_count);
    
    if (test_ctx.cell_count > 0 && test_ctx.clear_count > 0) {
        printf("\n✅ ALL TESTS PASSED!\n");
        return 0;
    } else {
        printf("\n❌ SOME TESTS FAILED\n");
        return 1;
    }
}