/* libtmuxcore_input.c - Keyboard input handling for tmux */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "libtmuxcore_api.h"

/* Key binding states */
typedef enum {
    INPUT_NORMAL,
    INPUT_PREFIX,    /* After Ctrl-B */
    INPUT_COMMAND,   /* In command mode (:) */
    INPUT_COPY       /* In copy mode */
} input_state_t;

/* Input context */
typedef struct {
    input_state_t state;
    char command_buffer[256];
    int command_pos;
    int prefix_timeout;  /* Timeout counter for prefix key */
    tmc_session_t current_session;
    tmc_window_t current_window;
    tmc_pane_t current_pane;
} tmc_input_context_t;

/* Global input context */
static tmc_input_context_t g_input_ctx = {
    .state = INPUT_NORMAL,
    .command_pos = 0,
    .prefix_timeout = 0
};

/* Key codes */
#define CTRL(c) ((c) & 0x1F)
#define ESC 0x1B
#define BACKSPACE 0x7F
#define ENTER 0x0D

/* Default prefix key (Ctrl-B) */
#define PREFIX_KEY CTRL('b')

/* External PTY functions */
extern void *tmc_pty_for_pane(tmc_pane_t pane);
extern ssize_t tmc_pty_write(void *pty, const char *data, size_t size);

/* External session functions */
extern tmc_session_t tmc_session_current_real(void);
extern tmc_window_t tmc_window_current_real(void);
extern tmc_pane_t tmc_pane_current_real(void);
extern tmc_error_t tmc_window_new_real(tmc_session_t session, const char *name, tmc_window_t *window);
extern tmc_error_t tmc_pane_split_real(tmc_window_t window, int horizontal, uint32_t size, tmc_pane_t *pane);

/* Reset input state to normal */
static void reset_input_state(void) {
    g_input_ctx.state = INPUT_NORMAL;
    g_input_ctx.command_pos = 0;
    g_input_ctx.command_buffer[0] = '\0';
    g_input_ctx.prefix_timeout = 0;
}

/* Process command mode input */
static void process_command_input(char key) {
    if (key == ENTER) {
        /* Execute command */
        g_input_ctx.command_buffer[g_input_ctx.command_pos] = '\0';
        printf("[INPUT] Executing command: %s\n", g_input_ctx.command_buffer);
        
        /* Simple command parsing */
        if (strncmp(g_input_ctx.command_buffer, "new-window", 10) == 0) {
            tmc_window_t window;
            tmc_window_new_real(g_input_ctx.current_session, "new", &window);
        } else if (strncmp(g_input_ctx.command_buffer, "split-window", 12) == 0) {
            tmc_pane_t pane;
            int horizontal = strstr(g_input_ctx.command_buffer, "-h") != NULL;
            tmc_pane_split_real(g_input_ctx.current_window, horizontal, 50, &pane);
        } else if (strcmp(g_input_ctx.command_buffer, "detach") == 0) {
            printf("[INPUT] Detaching from session\n");
        } else if (strcmp(g_input_ctx.command_buffer, "list-sessions") == 0) {
            tmc_command_execute("list-sessions");
        }
        
        reset_input_state();
    } else if (key == ESC) {
        /* Cancel command */
        reset_input_state();
    } else if (key == BACKSPACE && g_input_ctx.command_pos > 0) {
        /* Delete character */
        g_input_ctx.command_pos--;
        g_input_ctx.command_buffer[g_input_ctx.command_pos] = '\0';
    } else if (isprint(key) && g_input_ctx.command_pos < 255) {
        /* Add character */
        g_input_ctx.command_buffer[g_input_ctx.command_pos++] = key;
        g_input_ctx.command_buffer[g_input_ctx.command_pos] = '\0';
    }
}

/* Process prefix mode input */
static void process_prefix_input(char key) {
    printf("[INPUT] Prefix command: %c\n", key);
    
    switch (key) {
        case 'c':  /* New window */
            {
                tmc_window_t window;
                if (tmc_window_new_real(g_input_ctx.current_session, "shell", &window) == TMC_SUCCESS) {
                    printf("[INPUT] Created new window\n");
                    g_input_ctx.current_window = window;
                }
            }
            break;
            
        case '"':  /* Split horizontally */
            {
                tmc_pane_t pane;
                if (tmc_pane_split_real(g_input_ctx.current_window, 1, 50, &pane) == TMC_SUCCESS) {
                    printf("[INPUT] Split pane horizontally\n");
                    g_input_ctx.current_pane = pane;
                }
            }
            break;
            
        case '%':  /* Split vertically */
            {
                tmc_pane_t pane;
                if (tmc_pane_split_real(g_input_ctx.current_window, 0, 50, &pane) == TMC_SUCCESS) {
                    printf("[INPUT] Split pane vertically\n");
                    g_input_ctx.current_pane = pane;
                }
            }
            break;
            
        case 'd':  /* Detach */
            printf("[INPUT] Detaching from session\n");
            break;
            
        case 'x':  /* Kill pane */
            printf("[INPUT] Killing current pane\n");
            break;
            
        case 'n':  /* Next window */
            printf("[INPUT] Switching to next window\n");
            break;
            
        case 'p':  /* Previous window */
            printf("[INPUT] Switching to previous window\n");
            break;
            
        case '0'...'9':  /* Select window by number */
            printf("[INPUT] Selecting window %c\n", key);
            break;
            
        case ':':  /* Enter command mode */
            g_input_ctx.state = INPUT_COMMAND;
            g_input_ctx.command_pos = 0;
            g_input_ctx.command_buffer[0] = '\0';
            printf("[INPUT] Entering command mode\n");
            return;  /* Don't reset state */
            
        case '[':  /* Enter copy mode */
            g_input_ctx.state = INPUT_COPY;
            printf("[INPUT] Entering copy mode\n");
            return;  /* Don't reset state */
            
        case '?':  /* Show help */
            printf("[INPUT] Tmux key bindings:\n");
            printf("  c - new window\n");
            printf("  \" - split horizontally\n");
            printf("  %% - split vertically\n");
            printf("  d - detach\n");
            printf("  x - kill pane\n");
            printf("  n/p - next/previous window\n");
            printf("  0-9 - select window\n");
            printf("  : - command mode\n");
            printf("  [ - copy mode\n");
            break;
            
        case PREFIX_KEY:  /* Double prefix sends literal Ctrl-B */
            /* Send Ctrl-B to current pane */
            {
                void *pty = tmc_pty_for_pane(g_input_ctx.current_pane);
                if (pty) {
                    char ctrl_b = PREFIX_KEY;
                    tmc_pty_write(pty, &ctrl_b, 1);
                }
            }
            break;
            
        default:
            printf("[INPUT] Unknown prefix command: %c\n", key);
            break;
    }
    
    reset_input_state();
}

/* Main keyboard input handler */
tmc_error_t tmc_input_process_key(char key) {
    /* Update current context */
    g_input_ctx.current_session = tmc_session_current_real();
    g_input_ctx.current_window = tmc_window_current_real();
    g_input_ctx.current_pane = tmc_pane_current_real();
    
    switch (g_input_ctx.state) {
        case INPUT_NORMAL:
            if (key == PREFIX_KEY) {
                /* Enter prefix mode */
                g_input_ctx.state = INPUT_PREFIX;
                g_input_ctx.prefix_timeout = 100;  /* Timeout after ~1 second */
                printf("[INPUT] Prefix key pressed (Ctrl-B)\n");
            } else {
                /* Pass through to current pane */
                void *pty = tmc_pty_for_pane(g_input_ctx.current_pane);
                if (pty) {
                    tmc_pty_write(pty, &key, 1);
                }
            }
            break;
            
        case INPUT_PREFIX:
            process_prefix_input(key);
            break;
            
        case INPUT_COMMAND:
            process_command_input(key);
            break;
            
        case INPUT_COPY:
            if (key == 'q' || key == ESC) {
                /* Exit copy mode */
                reset_input_state();
                printf("[INPUT] Exited copy mode\n");
            } else {
                /* Handle copy mode navigation */
                printf("[INPUT] Copy mode: %c\n", key);
            }
            break;
    }
    
    return TMC_SUCCESS;
}

/* Process a string of input (for testing or batch input) */
tmc_error_t tmc_input_process_string(const char *input) {
    if (!input) {
        return TMC_ERROR_INVALID_PARAM;
    }
    
    while (*input) {
        tmc_input_process_key(*input);
        input++;
    }
    
    return TMC_SUCCESS;
}

/* Check for prefix timeout */
void tmc_input_tick(void) {
    if (g_input_ctx.state == INPUT_PREFIX && g_input_ctx.prefix_timeout > 0) {
        g_input_ctx.prefix_timeout--;
        if (g_input_ctx.prefix_timeout == 0) {
            printf("[INPUT] Prefix key timeout\n");
            reset_input_state();
        }
    }
}

/* Get current input state for UI display */
const char *tmc_input_get_state_string(void) {
    static char state_buffer[64];
    
    switch (g_input_ctx.state) {
        case INPUT_NORMAL:
            return "";
            
        case INPUT_PREFIX:
            return "[PREFIX]";
            
        case INPUT_COMMAND:
            snprintf(state_buffer, sizeof(state_buffer), ":%s", g_input_ctx.command_buffer);
            return state_buffer;
            
        case INPUT_COPY:
            return "[COPY MODE]";
    }
    
    return "";
}

/* Initialize input system */
void tmc_input_init(void) {
    reset_input_state();
    printf("[INPUT] Input system initialized (prefix key: Ctrl-B)\n");
}

/* Cleanup input system */
void tmc_input_cleanup(void) {
    reset_input_state();
    printf("[INPUT] Input system cleaned up\n");
}