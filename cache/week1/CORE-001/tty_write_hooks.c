// tty_write_hooks.c - Hook implementations for all tty_cmd_* functions
// Purpose: Extract and route all TTY output commands through backend interface
// Author: CORE-001 (c-tmux-specialist)
// Date: 2025-08-25
// Version: 1.0.0

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// Include tmux headers (these paths will be adjusted during integration)
struct tty;
struct tty_ctx;
struct grid_cell;

// Include our backend interface
#include "../ARCH-001/ui_backend.h"

// ============================================================================
// Hook Implementation Tracking
// ============================================================================

// Total of 22 tty_cmd_* functions identified in tmux/tty.c
#define TTY_CMD_COUNT 22

typedef struct {
    const char* name;
    void (*original)(struct tty*, const struct tty_ctx*);
    void (*hook)(struct ui_backend*, const struct tty_ctx*);
    bool intercepted;
} tty_cmd_hook_t;

// ============================================================================
// Global Backend Instance
// ============================================================================

static struct ui_backend* g_ui_backend = NULL;

// ============================================================================
// Hook Implementations (22 functions)
// ============================================================================

// Character/cell operations (5 functions)
static void hook_cmd_cell(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_cell) {
        backend->ops->cmd_cell(backend, ctx);
    }
}

static void hook_cmd_cells(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_cells) {
        backend->ops->cmd_cells(backend, ctx);
    }
}

static void hook_cmd_insertcharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_insertcharacter) {
        backend->ops->cmd_insertcharacter(backend, ctx);
    }
}

static void hook_cmd_deletecharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_deletecharacter) {
        backend->ops->cmd_deletecharacter(backend, ctx);
    }
}

static void hook_cmd_clearcharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_clearcharacter) {
        backend->ops->cmd_clearcharacter(backend, ctx);
    }
}

// Line operations (5 functions)
static void hook_cmd_insertline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_insertline) {
        backend->ops->cmd_insertline(backend, ctx);
    }
}

static void hook_cmd_deleteline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_deleteline) {
        backend->ops->cmd_deleteline(backend, ctx);
    }
}

static void hook_cmd_clearline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_clearline) {
        backend->ops->cmd_clearline(backend, ctx);
    }
}

static void hook_cmd_clearendofline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_clearendofline) {
        backend->ops->cmd_clearendofline(backend, ctx);
    }
}

static void hook_cmd_clearstartofline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_clearstartofline) {
        backend->ops->cmd_clearstartofline(backend, ctx);
    }
}

// Screen operations (4 functions)
static void hook_cmd_clearscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_clearscreen) {
        backend->ops->cmd_clearscreen(backend, ctx);
    }
}

static void hook_cmd_clearendofscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_clearendofscreen) {
        backend->ops->cmd_clearendofscreen(backend, ctx);
    }
}

static void hook_cmd_clearstartofscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_clearstartofscreen) {
        backend->ops->cmd_clearstartofscreen(backend, ctx);
    }
}

static void hook_cmd_alignmenttest(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_alignmenttest) {
        backend->ops->cmd_alignmenttest(backend, ctx);
    }
}

// Scrolling operations (4 functions)
static void hook_cmd_reverseindex(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_reverseindex) {
        backend->ops->cmd_reverseindex(backend, ctx);
    }
}

static void hook_cmd_linefeed(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_linefeed) {
        backend->ops->cmd_linefeed(backend, ctx);
    }
}

static void hook_cmd_scrollup(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_scrollup) {
        backend->ops->cmd_scrollup(backend, ctx);
    }
}

static void hook_cmd_scrolldown(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_scrolldown) {
        backend->ops->cmd_scrolldown(backend, ctx);
    }
}

// Special operations (4 functions)
static void hook_cmd_setselection(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_setselection) {
        backend->ops->cmd_setselection(backend, ctx);
    }
}

static void hook_cmd_rawstring(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_rawstring) {
        backend->ops->cmd_rawstring(backend, ctx);
    }
}

static void hook_cmd_sixelimage(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_sixelimage) {
        backend->ops->cmd_sixelimage(backend, ctx);
    }
}

static void hook_cmd_syncstart(struct ui_backend* backend, const struct tty_ctx* ctx) {
    if (backend && backend->ops && backend->ops->cmd_syncstart) {
        backend->ops->cmd_syncstart(backend, ctx);
    }
}

// ============================================================================
// Function Mapping Table
// ============================================================================

// Function pointers to original implementations (will be filled during runtime)
static void (*orig_tty_cmd_cell)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_cells)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_insertcharacter)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_deletecharacter)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_clearcharacter)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_insertline)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_deleteline)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_clearline)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_clearendofline)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_clearstartofline)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_clearscreen)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_clearendofscreen)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_clearstartofscreen)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_alignmenttest)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_reverseindex)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_linefeed)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_scrollup)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_scrolldown)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_setselection)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_rawstring)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_sixelimage)(struct tty*, const struct tty_ctx*) = NULL;
static void (*orig_tty_cmd_syncstart)(struct tty*, const struct tty_ctx*) = NULL;

// Hook mapping table
static tty_cmd_hook_t tty_cmd_hooks[TTY_CMD_COUNT] = {
    // Character/cell operations
    {"tty_cmd_cell",            NULL, hook_cmd_cell,            false},
    {"tty_cmd_cells",           NULL, hook_cmd_cells,           false},
    {"tty_cmd_insertcharacter", NULL, hook_cmd_insertcharacter, false},
    {"tty_cmd_deletecharacter", NULL, hook_cmd_deletecharacter, false},
    {"tty_cmd_clearcharacter",  NULL, hook_cmd_clearcharacter,  false},
    
    // Line operations
    {"tty_cmd_insertline",      NULL, hook_cmd_insertline,      false},
    {"tty_cmd_deleteline",      NULL, hook_cmd_deleteline,      false},
    {"tty_cmd_clearline",       NULL, hook_cmd_clearline,       false},
    {"tty_cmd_clearendofline",  NULL, hook_cmd_clearendofline,  false},
    {"tty_cmd_clearstartofline",NULL, hook_cmd_clearstartofline,false},
    
    // Screen operations
    {"tty_cmd_clearscreen",     NULL, hook_cmd_clearscreen,     false},
    {"tty_cmd_clearendofscreen",NULL, hook_cmd_clearendofscreen,false},
    {"tty_cmd_clearstartofscreen",NULL,hook_cmd_clearstartofscreen,false},
    {"tty_cmd_alignmenttest",   NULL, hook_cmd_alignmenttest,   false},
    
    // Scrolling operations
    {"tty_cmd_reverseindex",    NULL, hook_cmd_reverseindex,    false},
    {"tty_cmd_linefeed",        NULL, hook_cmd_linefeed,        false},
    {"tty_cmd_scrollup",        NULL, hook_cmd_scrollup,        false},
    {"tty_cmd_scrolldown",      NULL, hook_cmd_scrolldown,      false},
    
    // Special operations
    {"tty_cmd_setselection",    NULL, hook_cmd_setselection,    false},
    {"tty_cmd_rawstring",       NULL, hook_cmd_rawstring,       false},
    {"tty_cmd_sixelimage",      NULL, hook_cmd_sixelimage,      false},
    {"tty_cmd_syncstart",       NULL, hook_cmd_syncstart,       false},
};

// ============================================================================
// Intercepted Wrapper Functions
// ============================================================================

// These functions replace the original tty_cmd_* functions
// They route through the backend if available, otherwise call original

void tty_cmd_cell(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_cell(g_ui_backend, ctx);
    } else if (orig_tty_cmd_cell) {
        orig_tty_cmd_cell(tty, ctx);
    }
}

void tty_cmd_cells(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_cells(g_ui_backend, ctx);
    } else if (orig_tty_cmd_cells) {
        orig_tty_cmd_cells(tty, ctx);
    }
}

void tty_cmd_insertcharacter(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_insertcharacter(g_ui_backend, ctx);
    } else if (orig_tty_cmd_insertcharacter) {
        orig_tty_cmd_insertcharacter(tty, ctx);
    }
}

void tty_cmd_deletecharacter(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_deletecharacter(g_ui_backend, ctx);
    } else if (orig_tty_cmd_deletecharacter) {
        orig_tty_cmd_deletecharacter(tty, ctx);
    }
}

void tty_cmd_clearcharacter(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_clearcharacter(g_ui_backend, ctx);
    } else if (orig_tty_cmd_clearcharacter) {
        orig_tty_cmd_clearcharacter(tty, ctx);
    }
}

void tty_cmd_insertline(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_insertline(g_ui_backend, ctx);
    } else if (orig_tty_cmd_insertline) {
        orig_tty_cmd_insertline(tty, ctx);
    }
}

void tty_cmd_deleteline(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_deleteline(g_ui_backend, ctx);
    } else if (orig_tty_cmd_deleteline) {
        orig_tty_cmd_deleteline(tty, ctx);
    }
}

void tty_cmd_clearline(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_clearline(g_ui_backend, ctx);
    } else if (orig_tty_cmd_clearline) {
        orig_tty_cmd_clearline(tty, ctx);
    }
}

void tty_cmd_clearendofline(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_clearendofline(g_ui_backend, ctx);
    } else if (orig_tty_cmd_clearendofline) {
        orig_tty_cmd_clearendofline(tty, ctx);
    }
}

void tty_cmd_clearstartofline(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_clearstartofline(g_ui_backend, ctx);
    } else if (orig_tty_cmd_clearstartofline) {
        orig_tty_cmd_clearstartofline(tty, ctx);
    }
}

void tty_cmd_clearscreen(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_clearscreen(g_ui_backend, ctx);
    } else if (orig_tty_cmd_clearscreen) {
        orig_tty_cmd_clearscreen(tty, ctx);
    }
}

void tty_cmd_clearendofscreen(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_clearendofscreen(g_ui_backend, ctx);
    } else if (orig_tty_cmd_clearendofscreen) {
        orig_tty_cmd_clearendofscreen(tty, ctx);
    }
}

void tty_cmd_clearstartofscreen(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_clearstartofscreen(g_ui_backend, ctx);
    } else if (orig_tty_cmd_clearstartofscreen) {
        orig_tty_cmd_clearstartofscreen(tty, ctx);
    }
}

void tty_cmd_alignmenttest(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_alignmenttest(g_ui_backend, ctx);
    } else if (orig_tty_cmd_alignmenttest) {
        orig_tty_cmd_alignmenttest(tty, ctx);
    }
}

void tty_cmd_reverseindex(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_reverseindex(g_ui_backend, ctx);
    } else if (orig_tty_cmd_reverseindex) {
        orig_tty_cmd_reverseindex(tty, ctx);
    }
}

void tty_cmd_linefeed(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_linefeed(g_ui_backend, ctx);
    } else if (orig_tty_cmd_linefeed) {
        orig_tty_cmd_linefeed(tty, ctx);
    }
}

void tty_cmd_scrollup(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_scrollup(g_ui_backend, ctx);
    } else if (orig_tty_cmd_scrollup) {
        orig_tty_cmd_scrollup(tty, ctx);
    }
}

void tty_cmd_scrolldown(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_scrolldown(g_ui_backend, ctx);
    } else if (orig_tty_cmd_scrolldown) {
        orig_tty_cmd_scrolldown(tty, ctx);
    }
}

void tty_cmd_setselection(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_setselection(g_ui_backend, ctx);
    } else if (orig_tty_cmd_setselection) {
        orig_tty_cmd_setselection(tty, ctx);
    }
}

void tty_cmd_rawstring(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_rawstring(g_ui_backend, ctx);
    } else if (orig_tty_cmd_rawstring) {
        orig_tty_cmd_rawstring(tty, ctx);
    }
}

void tty_cmd_sixelimage(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_sixelimage(g_ui_backend, ctx);
    } else if (orig_tty_cmd_sixelimage) {
        orig_tty_cmd_sixelimage(tty, ctx);
    }
}

void tty_cmd_syncstart(struct tty* tty, const struct tty_ctx* ctx) {
    if (g_ui_backend) {
        hook_cmd_syncstart(g_ui_backend, ctx);
    } else if (orig_tty_cmd_syncstart) {
        orig_tty_cmd_syncstart(tty, ctx);
    }
}

// ============================================================================
// Hook Installation/Removal Functions
// ============================================================================

int tty_hooks_install(struct ui_backend* backend) {
    if (!backend) {
        return -1;
    }
    
    // Store backend reference
    g_ui_backend = backend;
    
    // Mark all hooks as intercepted
    for (int i = 0; i < TTY_CMD_COUNT; i++) {
        tty_cmd_hooks[i].intercepted = true;
    }
    
    return 0;
}

int tty_hooks_uninstall(void) {
    // Clear backend reference
    g_ui_backend = NULL;
    
    // Mark all hooks as not intercepted
    for (int i = 0; i < TTY_CMD_COUNT; i++) {
        tty_cmd_hooks[i].intercepted = false;
    }
    
    return 0;
}

// ============================================================================
// Hook Statistics and Debugging
// ============================================================================

typedef struct {
    uint64_t call_count[TTY_CMD_COUNT];
    uint64_t total_calls;
    uint64_t intercepted_calls;
    uint64_t fallback_calls;
} tty_hook_stats_t;

static tty_hook_stats_t g_hook_stats = {0};

void tty_hooks_get_stats(tty_hook_stats_t* stats) {
    if (stats) {
        memcpy(stats, &g_hook_stats, sizeof(tty_hook_stats_t));
    }
}

void tty_hooks_reset_stats(void) {
    memset(&g_hook_stats, 0, sizeof(tty_hook_stats_t));
}

const char* tty_hooks_get_function_name(int index) {
    if (index >= 0 && index < TTY_CMD_COUNT) {
        return tty_cmd_hooks[index].name;
    }
    return NULL;
}

int tty_hooks_get_count(void) {
    return TTY_CMD_COUNT;
}

// ============================================================================
// Initialization Function
// ============================================================================

void tty_hooks_init(void) {
    // Initialize function pointers to originals
    // These would be set during runtime linking
    tty_cmd_hooks[0].original = orig_tty_cmd_cell;
    tty_cmd_hooks[1].original = orig_tty_cmd_cells;
    tty_cmd_hooks[2].original = orig_tty_cmd_insertcharacter;
    tty_cmd_hooks[3].original = orig_tty_cmd_deletecharacter;
    tty_cmd_hooks[4].original = orig_tty_cmd_clearcharacter;
    tty_cmd_hooks[5].original = orig_tty_cmd_insertline;
    tty_cmd_hooks[6].original = orig_tty_cmd_deleteline;
    tty_cmd_hooks[7].original = orig_tty_cmd_clearline;
    tty_cmd_hooks[8].original = orig_tty_cmd_clearendofline;
    tty_cmd_hooks[9].original = orig_tty_cmd_clearstartofline;
    tty_cmd_hooks[10].original = orig_tty_cmd_clearscreen;
    tty_cmd_hooks[11].original = orig_tty_cmd_clearendofscreen;
    tty_cmd_hooks[12].original = orig_tty_cmd_clearstartofscreen;
    tty_cmd_hooks[13].original = orig_tty_cmd_alignmenttest;
    tty_cmd_hooks[14].original = orig_tty_cmd_reverseindex;
    tty_cmd_hooks[15].original = orig_tty_cmd_linefeed;
    tty_cmd_hooks[16].original = orig_tty_cmd_scrollup;
    tty_cmd_hooks[17].original = orig_tty_cmd_scrolldown;
    tty_cmd_hooks[18].original = orig_tty_cmd_setselection;
    tty_cmd_hooks[19].original = orig_tty_cmd_rawstring;
    tty_cmd_hooks[20].original = orig_tty_cmd_sixelimage;
    tty_cmd_hooks[21].original = orig_tty_cmd_syncstart;
}