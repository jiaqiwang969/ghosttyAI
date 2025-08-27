/* test_ui_integration.c - Test UI callback integration with libtmuxcore */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>

typedef unsigned int u_int;
typedef void* tmc_session_t;
typedef void* tmc_window_t;
typedef void* tmc_pane_t;

#define TMC_SUCCESS 0

/* Function pointers from dynamic library */
int (*p_tmc_init)(void);
void (*p_tmc_cleanup)(void);
int (*p_tmc_session_new_real)(const char*, tmc_session_t*);
int (*p_tmc_session_attach_real)(tmc_session_t);
int (*p_tmc_window_new_real)(tmc_session_t, const char*, tmc_window_t*);
int (*p_tmc_pane_split_real)(tmc_window_t, int, u_int, tmc_pane_t*);
int (*p_tmc_command_execute_real)(const char*);

/* UI callback functions from library */
void (*p_ui_callbacks_register)(void*, void (*)(void*));
void (*p_ui_grid_init)(int, int);
void (*p_ui_get_grid_cell)(int, int, char*, int*, int*, int*);
void (*p_ui_get_cursor_pos)(int*, int*);
void (*p_ui_debug_print_grid)(void);

/* Update callback for UI changes */
static int update_count = 0;
static void on_grid_update(void *user_data) {
    update_count++;
    printf("[Test] Grid updated (update #%d)\n", update_count);
}

/* Load symbols from dynamic library */
int load_symbols(void *handle) {
    p_tmc_init = dlsym(handle, "tmc_init");
    p_tmc_cleanup = dlsym(handle, "tmc_cleanup");
    p_tmc_session_new_real = dlsym(handle, "tmc_session_new_real");
    p_tmc_session_attach_real = dlsym(handle, "tmc_session_attach_real");
    p_tmc_window_new_real = dlsym(handle, "tmc_window_new_real");
    p_tmc_pane_split_real = dlsym(handle, "tmc_pane_split_real");
    p_tmc_command_execute_real = dlsym(handle, "tmc_command_execute_real");
    
    p_ui_callbacks_register = dlsym(handle, "ui_callbacks_register");
    p_ui_grid_init = dlsym(handle, "ui_grid_init");
    p_ui_get_grid_cell = dlsym(handle, "ui_get_grid_cell");
    p_ui_get_cursor_pos = dlsym(handle, "ui_get_cursor_pos");
    p_ui_debug_print_grid = dlsym(handle, "ui_debug_print_grid");
    
    if (!p_tmc_init || !p_ui_callbacks_register) {
        printf("ERROR: Failed to load core symbols\n");
        return -1;
    }
    
    return 0;
}

/* Simulate terminal operations */
void simulate_terminal_output(void) {
    printf("\n=== Simulating Terminal Operations ===\n");
    
    /* This would normally trigger UI callbacks */
    /* For now, we'll just create sessions and windows */
    tmc_session_t session;
    tmc_window_t window;
    tmc_pane_t pane;
    
    /* Create a session */
    if (p_tmc_session_new_real("test-session", &session) == TMC_SUCCESS) {
        printf("✓ Created session\n");
        
        /* Attach to session */
        if (p_tmc_session_attach_real(session) == TMC_SUCCESS) {
            printf("✓ Attached to session\n");
            
            /* Create window */
            if (p_tmc_window_new_real(session, "test-window", &window) == TMC_SUCCESS) {
                printf("✓ Created window\n");
                
                /* Split pane */
                if (p_tmc_pane_split_real(window, 1, 50, &pane) == TMC_SUCCESS) {
                    printf("✓ Split pane horizontally\n");
                }
            }
        }
    }
    
    /* Execute command to list structure */
    p_tmc_command_execute_real("list-sessions");
    p_tmc_command_execute_real("list-windows");
    p_tmc_command_execute_real("list-panes");
}

/* Test grid rendering */
void test_grid_rendering(void) {
    printf("\n=== Testing Grid Rendering ===\n");
    
    /* Check grid state */
    int cursor_x, cursor_y;
    p_ui_get_cursor_pos(&cursor_x, &cursor_y);
    printf("Cursor position: (%d, %d)\n", cursor_x, cursor_y);
    
    /* Sample some grid cells */
    for (int y = 0; y < 3; y++) {
        printf("Row %d: ", y);
        for (int x = 0; x < 20; x++) {
            char ch;
            int fg, bg, attrs;
            p_ui_get_grid_cell(x, y, &ch, &fg, &bg, &attrs);
            putchar(ch >= 32 && ch < 127 ? ch : '.');
        }
        printf("\n");
    }
    
    /* Debug print */
    if (p_ui_debug_print_grid) {
        p_ui_debug_print_grid();
    }
}

int main() {
    printf("=== UI Integration Test for libtmuxcore ===\n\n");
    
    /* Load dynamic library */
    void *handle = dlopen("./libtmuxcore.dylib", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "ERROR: Cannot load library: %s\n", dlerror());
        return 1;
    }
    
    /* Load symbols */
    if (load_symbols(handle) != 0) {
        dlclose(handle);
        return 1;
    }
    
    /* Initialize tmux core */
    if (p_tmc_init() != TMC_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to initialize tmux core\n");
        dlclose(handle);
        return 1;
    }
    printf("✓ Initialized tmux core\n");
    
    /* Initialize UI grid */
    if (p_ui_grid_init) {
        p_ui_grid_init(80, 24);
        printf("✓ Initialized UI grid (80x24)\n");
    }
    
    /* Register UI callbacks */
    if (p_ui_callbacks_register) {
        p_ui_callbacks_register(&update_count, on_grid_update);
        printf("✓ Registered UI callbacks\n");
    }
    
    /* Simulate terminal operations */
    simulate_terminal_output();
    
    /* Test grid rendering */
    test_grid_rendering();
    
    /* Report update count */
    printf("\n=== Test Results ===\n");
    printf("Grid updates triggered: %d\n", update_count);
    
    /* Cleanup */
    p_tmc_cleanup();
    printf("✓ Cleaned up tmux core\n");
    
    dlclose(handle);
    
    printf("\n=== UI Integration Test PASSED ===\n");
    return 0;
}