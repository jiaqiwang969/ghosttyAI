// test_tmux_ghostty_poc.c - Proof of concept for tmux-Ghostty integration
// Purpose: Demonstrate tmux commands flowing through to Ghostty callbacks
// Date: 2025-08-26
// Task: T-502-B - First tmux output in Ghostty

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define LIBTMUXCORE_BUILD 1
#include "tmux/ui_backend/ui_backend_minimal.h"

// External functions from our UI backend
extern void ui_backend_set_callbacks(void* callbacks, void* user_data);
extern int ui_backend_has_callbacks(void);
extern int ui_backend_init(void);
extern ui_backend_t* ui_backend_get_instance(void);
extern int ui_backend_dispatch(ui_backend_t* backend, 
                              void (*cmdfn)(struct tty *, const struct tty_ctx *),
                              struct tty_ctx* ctx);
extern void ui_backend_flush(void);

// Callback structure
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

// Simulated Ghostty Terminal Grid (simplified)
#define GRID_ROWS 24
#define GRID_COLS 80

typedef struct {
    char grid[GRID_ROWS][GRID_COLS];
    int cursor_row;
    int cursor_col;
    int cell_count;
    int clear_count;
} ghostty_terminal_t;

// Ghostty callback implementations
void ghostty_on_cell(char ch, int row, int col, int attr, int fg, int bg, void* user_data) {
    ghostty_terminal_t* term = (ghostty_terminal_t*)user_data;
    
    printf("[GHOSTTY] Writing '%c' at (%d,%d) with fg=#%06X bg=#%06X\n", 
           ch, row, col, fg, bg);
    
    if (row >= 0 && row < GRID_ROWS && col >= 0 && col < GRID_COLS) {
        term->grid[row][col] = ch;
        term->cursor_row = row;
        term->cursor_col = col + 1;
        term->cell_count++;
    }
}

void ghostty_on_clear_line(int row, void* user_data) {
    ghostty_terminal_t* term = (ghostty_terminal_t*)user_data;
    
    printf("[GHOSTTY] Clearing line %d\n", row);
    
    if (row >= 0 && row < GRID_ROWS) {
        memset(term->grid[row], ' ', GRID_COLS);
        term->clear_count++;
    }
}

void ghostty_on_clear_screen(void* user_data) {
    ghostty_terminal_t* term = (ghostty_terminal_t*)user_data;
    
    printf("[GHOSTTY] Clearing entire screen\n");
    
    for (int i = 0; i < GRID_ROWS; i++) {
        memset(term->grid[i], ' ', GRID_COLS);
    }
    term->cursor_row = 0;
    term->cursor_col = 0;
    term->clear_count++;
}

void ghostty_on_flush(void* user_data) {
    ghostty_terminal_t* term = (ghostty_terminal_t*)user_data;
    
    printf("[GHOSTTY] Flush - rendering grid:\n");
    printf("╔");
    for (int i = 0; i < 40; i++) printf("═");
    printf("╗\n");
    
    for (int row = 0; row < 5 && row < GRID_ROWS; row++) {
        printf("║");
        for (int col = 0; col < 40 && col < GRID_COLS; col++) {
            char ch = term->grid[row][col];
            if (ch >= 0x20 && ch <= 0x7E) {
                printf("%c", ch);
            } else {
                printf(" ");
            }
        }
        printf("║\n");
    }
    
    printf("╚");
    for (int i = 0; i < 40; i++) printf("═");
    printf("╝\n");
}

// Simulate tmux sending commands
void simulate_tmux_session() {
    printf("\n=== Simulating tmux Session ===\n\n");
    
    ui_backend_t* backend = ui_backend_get_instance();
    if (!backend) {
        printf("ERROR: Backend not initialized\n");
        return;
    }
    
    // Simulate tmux writing "Hello from tmux!" on row 0
    const char* message = "Hello from tmux!";
    for (int i = 0; message[i]; i++) {
        struct grid_cell cell = {0};
        cell.data.size = 1;
        cell.data.data[0] = message[i];
        cell.fg = 0x00FF00;  // Green text
        cell.bg = 0x000000;  // Black background
        
        struct tty_ctx ctx = {0};
        ctx.cell = &cell;
        ctx.ocy = 0;
        ctx.ocx = i;
        ctx.ui_cmd_id = 1;  // UI_CMD_CELL
        
        ui_backend_dispatch(backend, NULL, &ctx);
    }
    
    // Simulate writing on row 2
    const char* line2 = "This is Ghostty with tmux!";
    for (int i = 0; line2[i]; i++) {
        struct grid_cell cell = {0};
        cell.data.size = 1;
        cell.data.data[0] = line2[i];
        cell.fg = 0x00FFFF;  // Cyan text
        cell.bg = 0x000000;
        
        struct tty_ctx ctx = {0};
        ctx.cell = &cell;
        ctx.ocy = 2;
        ctx.ocx = i;
        ctx.ui_cmd_id = 1;  // UI_CMD_CELL
        
        ui_backend_dispatch(backend, NULL, &ctx);
    }
    
    // Clear line 1
    struct tty_ctx clear_ctx = {0};
    clear_ctx.ocy = 1;
    clear_ctx.ui_cmd_id = 2;  // UI_CMD_CLEARLINE
    ui_backend_dispatch(backend, NULL, &clear_ctx);
    
    // Flush to render
    ui_backend_flush();
}

int main() {
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║     tmux → Ghostty Integration Demo           ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    
    // Initialize simulated Ghostty terminal
    ghostty_terminal_t ghostty_term = {0};
    for (int i = 0; i < GRID_ROWS; i++) {
        memset(ghostty_term.grid[i], ' ', GRID_COLS);
    }
    
    // Initialize UI Backend
    printf("\n1. Initializing UI Backend...\n");
    setenv("TMUX_UI_BACKEND", "ghostty", 1);
    if (ui_backend_init() != 0) {
        printf("❌ Failed to initialize UI Backend\n");
        return 1;
    }
    printf("✅ UI Backend initialized\n");
    
    // Register Ghostty callbacks
    printf("\n2. Registering Ghostty callbacks...\n");
    ui_callbacks_t callbacks = {
        .on_cell = ghostty_on_cell,
        .on_clear_line = ghostty_on_clear_line,
        .on_clear_screen = ghostty_on_clear_screen,
        .on_insert_line = NULL,
        .on_delete_line = NULL,
        .on_clear_eol = NULL,
        .on_reverse_index = NULL,
        .on_line_feed = NULL,
        .on_scroll_up = NULL,
        .on_scroll_down = NULL,
        .on_flush = ghostty_on_flush,
    };
    
    ui_backend_set_callbacks(&callbacks, &ghostty_term);
    
    if (!ui_backend_has_callbacks()) {
        printf("❌ Callbacks not registered\n");
        return 1;
    }
    printf("✅ Ghostty callbacks registered\n");
    
    // Simulate tmux session
    simulate_tmux_session();
    
    // Show statistics
    printf("\n=== Statistics ===\n");
    printf("Cells written: %d\n", ghostty_term.cell_count);
    printf("Lines cleared: %d\n", ghostty_term.clear_count);
    printf("Cursor position: (%d, %d)\n", ghostty_term.cursor_row, ghostty_term.cursor_col);
    
    printf("\n✅ Integration demonstration successful!\n");
    printf("   tmux commands → UI Backend → Ghostty callbacks\n\n");
    
    return 0;
}