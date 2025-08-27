/* libtmuxcore_session.c - Real session management implementation */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libtmuxcore_api.h"

/* Session structure */
typedef struct tmc_session_impl {
    char *name;
    struct tmc_window_impl *windows;
    struct tmc_window_impl *current_window;
    int window_count;
    time_t created;
    struct tmc_session_impl *next;
} tmc_session_impl_t;

/* Window structure */
typedef struct tmc_window_impl {
    char *name;
    int index;
    struct tmc_pane_impl *panes;
    struct tmc_pane_impl *active_pane;
    int pane_count;
    tmc_session_impl_t *session;
    struct tmc_window_impl *next;
} tmc_window_impl_t;

/* Pane structure */
typedef struct tmc_pane_impl {
    int id;
    int x, y;
    int width, height;
    char *cmd;
    tmc_window_impl_t *window;
    struct tmc_pane_impl *next;
} tmc_pane_impl_t;

/* Global session list */
static tmc_session_impl_t *g_sessions = NULL;
static tmc_session_impl_t *g_current_session = NULL;
static int g_next_pane_id = 1;

/* Helper functions */
static tmc_session_impl_t *find_session(const char *name) {
    tmc_session_impl_t *s;
    for (s = g_sessions; s; s = s->next) {
        if (strcmp(s->name, name) == 0) {
            return s;
        }
    }
    return NULL;
}

/* Forward declarations */
tmc_error_t tmc_window_new_real(tmc_session_t session, const char *name, tmc_window_t *window);

/* Session management functions */
tmc_error_t tmc_session_new_real(const char *name, tmc_session_t *session) {
    if (!name || !session) {
        return TMC_ERROR_INVALID_PARAM;
    }
    
    /* Check if session already exists */
    if (find_session(name)) {
        return TMC_ERROR_ALREADY_EXISTS;
    }
    
    /* Create new session */
    tmc_session_impl_t *s = calloc(1, sizeof(tmc_session_impl_t));
    if (!s) {
        return TMC_ERROR_OUT_OF_MEMORY;
    }
    
    s->name = strdup(name);
    s->created = time(NULL);
    s->window_count = 0;
    s->windows = NULL;
    s->current_window = NULL;
    
    /* Add to global list */
    s->next = g_sessions;
    g_sessions = s;
    
    *session = (tmc_session_t)s;
    
    printf("[TMC] Created real session: %s (created: %ld)\n", s->name, s->created);
    
    /* Create default window */
    tmc_window_t default_window;
    tmc_window_new_real((tmc_session_t)s, "shell", &default_window);
    
    return TMC_SUCCESS;
}

tmc_error_t tmc_session_attach_real(tmc_session_t session) {
    if (!session) {
        return TMC_ERROR_INVALID_PARAM;
    }
    
    tmc_session_impl_t *s = (tmc_session_impl_t *)session;
    g_current_session = s;
    
    printf("[TMC] Attached to session: %s (windows: %d)\n", s->name, s->window_count);
    return TMC_SUCCESS;
}

tmc_error_t tmc_session_detach_real(tmc_session_t session) {
    if (!session) {
        return TMC_ERROR_INVALID_PARAM;
    }
    
    if (g_current_session == (tmc_session_impl_t *)session) {
        g_current_session = NULL;
    }
    
    printf("[TMC] Detached from session\n");
    return TMC_SUCCESS;
}

tmc_error_t tmc_session_destroy_real(tmc_session_t session) {
    if (!session) {
        return TMC_ERROR_INVALID_PARAM;
    }
    
    tmc_session_impl_t *s = (tmc_session_impl_t *)session;
    
    /* Free windows */
    tmc_window_impl_t *w = s->windows;
    while (w) {
        tmc_window_impl_t *next = w->next;
        
        /* Free panes */
        tmc_pane_impl_t *p = w->panes;
        while (p) {
            tmc_pane_impl_t *next_pane = p->next;
            free(p->cmd);
            free(p);
            p = next_pane;
        }
        
        free(w->name);
        free(w);
        w = next;
    }
    
    /* Remove from global list */
    tmc_session_impl_t **pp = &g_sessions;
    while (*pp) {
        if (*pp == s) {
            *pp = s->next;
            break;
        }
        pp = &(*pp)->next;
    }
    
    free(s->name);
    free(s);
    
    return TMC_SUCCESS;
}

tmc_session_t tmc_session_current_real(void) {
    return (tmc_session_t)g_current_session;
}

/* Window management */
tmc_error_t tmc_window_new_real(tmc_session_t session, const char *name, tmc_window_t *window) {
    if (!session || !name || !window) {
        return TMC_ERROR_INVALID_PARAM;
    }
    
    tmc_session_impl_t *s = (tmc_session_impl_t *)session;
    
    /* Create window */
    tmc_window_impl_t *w = calloc(1, sizeof(tmc_window_impl_t));
    if (!w) {
        return TMC_ERROR_OUT_OF_MEMORY;
    }
    
    w->name = strdup(name);
    w->index = s->window_count++;
    w->session = s;
    w->panes = NULL;
    w->active_pane = NULL;
    w->pane_count = 0;
    
    /* Add to session */
    w->next = s->windows;
    s->windows = w;
    
    if (!s->current_window) {
        s->current_window = w;
    }
    
    *window = (tmc_window_t)w;
    
    printf("[TMC] Created window: %s (index: %d) in session: %s\n", 
           w->name, w->index, s->name);
    
    /* Create default pane */
    tmc_pane_impl_t *p = calloc(1, sizeof(tmc_pane_impl_t));
    if (p) {
        p->id = g_next_pane_id++;
        p->x = 0;
        p->y = 0;
        p->width = 80;
        p->height = 24;
        p->window = w;
        p->cmd = strdup("/bin/sh");
        
        w->panes = p;
        w->active_pane = p;
        w->pane_count = 1;
        
        printf("[TMC] Created default pane %d in window %s\n", p->id, w->name);
    }
    
    return TMC_SUCCESS;
}

tmc_window_t tmc_window_current_real(void) {
    if (g_current_session) {
        return (tmc_window_t)g_current_session->current_window;
    }
    return NULL;
}

/* Pane management */
tmc_error_t tmc_pane_split_real(tmc_window_t window, int horizontal, 
                                uint32_t size_percent, tmc_pane_t *new_pane) {
    if (!window || !new_pane) {
        return TMC_ERROR_INVALID_PARAM;
    }
    
    tmc_window_impl_t *w = (tmc_window_impl_t *)window;
    
    if (!w->active_pane) {
        return TMC_ERROR_NOT_FOUND;
    }
    
    /* Create new pane */
    tmc_pane_impl_t *p = calloc(1, sizeof(tmc_pane_impl_t));
    if (!p) {
        return TMC_ERROR_OUT_OF_MEMORY;
    }
    
    p->id = g_next_pane_id++;
    p->window = w;
    p->cmd = strdup("/bin/sh");
    
    /* Calculate dimensions based on split */
    tmc_pane_impl_t *active = w->active_pane;
    if (horizontal) {
        /* Horizontal split - divide height */
        int new_height = active->height * size_percent / 100;
        p->x = active->x;
        p->y = active->y + (active->height - new_height);
        p->width = active->width;
        p->height = new_height;
        active->height -= new_height;
    } else {
        /* Vertical split - divide width */
        int new_width = active->width * size_percent / 100;
        p->x = active->x + (active->width - new_width);
        p->y = active->y;
        p->width = new_width;
        p->height = active->height;
        active->width -= new_width;
    }
    
    /* Add to window */
    p->next = w->panes;
    w->panes = p;
    w->pane_count++;
    w->active_pane = p;
    
    *new_pane = (tmc_pane_t)p;
    
    printf("[TMC] Split pane %d (%s, %d%%) -> new pane %d at (%d,%d) %dx%d\n",
           active->id, horizontal ? "horizontal" : "vertical", size_percent,
           p->id, p->x, p->y, p->width, p->height);
    
    return TMC_SUCCESS;
}

tmc_pane_t tmc_pane_current_real(void) {
    if (g_current_session && g_current_session->current_window) {
        return (tmc_pane_t)g_current_session->current_window->active_pane;
    }
    return NULL;
}

/* Command execution */
tmc_error_t tmc_command_execute_real(const char *command) {
    if (!command) {
        return TMC_ERROR_INVALID_PARAM;
    }
    
    printf("[TMC] Executing command: %s\n", command);
    
    /* Parse and execute tmux-style commands */
    if (strncmp(command, "list-sessions", 13) == 0) {
        printf("[TMC] Sessions:\n");
        tmc_session_impl_t *s;
        for (s = g_sessions; s; s = s->next) {
            printf("  %s: %d windows (created: %s", 
                   s->name, s->window_count, ctime(&s->created));
            if (s == g_current_session) {
                printf(" (attached)");
            }
            printf(")\n");
        }
    } else if (strncmp(command, "list-windows", 12) == 0) {
        if (g_current_session) {
            printf("[TMC] Windows in session '%s':\n", g_current_session->name);
            tmc_window_impl_t *w;
            for (w = g_current_session->windows; w; w = w->next) {
                printf("  %d: %s (%d panes)", w->index, w->name, w->pane_count);
                if (w == g_current_session->current_window) {
                    printf(" (active)");
                }
                printf("\n");
            }
        }
    } else if (strncmp(command, "list-panes", 10) == 0) {
        if (g_current_session && g_current_session->current_window) {
            tmc_window_impl_t *w = g_current_session->current_window;
            printf("[TMC] Panes in window '%s':\n", w->name);
            tmc_pane_impl_t *p;
            for (p = w->panes; p; p = p->next) {
                printf("  %d: [%d,%d %dx%d] %s", 
                       p->id, p->x, p->y, p->width, p->height, p->cmd);
                if (p == w->active_pane) {
                    printf(" (active)");
                }
                printf("\n");
            }
        }
    }
    
    return TMC_SUCCESS;
}