// tty_write_hooks.h - Header for TTY write hook implementations
// Purpose: Public interface for tty_cmd_* function interception
// Author: CORE-001 (c-tmux-specialist)
// Date: 2025-08-25
// Version: 1.1.0 - Fixed interface naming (DEFECT-002)

#ifndef TTY_WRITE_HOOKS_H
#define TTY_WRITE_HOOKS_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
struct ui_backend;
struct tty;
struct tty_ctx;

// ============================================================================
// Hook Management Functions
// ============================================================================

// Initialize the hook system
void tty_hooks_init(void);

// Install hooks to redirect output through backend
// Returns 0 on success, -1 on error
int tty_hooks_install(struct ui_backend* backend);

// Uninstall hooks and restore original behavior
// Returns 0 on success, -1 on error
int tty_hooks_uninstall(void);

// ============================================================================
// Compatibility Macros (Transition Period)
// ============================================================================

// These macros provide backward compatibility during transition
// They map old names to new standardized names
#ifndef TTY_HOOKS_NO_COMPAT
  #define tty_write_hooks_init           tty_hooks_init
  #define tty_write_hooks_install        tty_hooks_install
  #define tty_write_hooks_uninstall      tty_hooks_uninstall
  #define tty_write_hooks_get_stats      tty_hooks_get_stats
  #define tty_write_hooks_reset_stats    tty_hooks_reset_stats
  
  // Deprecated warning can be enabled with TTY_HOOKS_WARN_DEPRECATED
  #ifdef TTY_HOOKS_WARN_DEPRECATED
    #warning "tty_write_hooks_* names are deprecated. Use tty_hooks_* instead."
  #endif
#endif

// ============================================================================
// Statistics and Debugging
// ============================================================================

typedef struct {
    uint64_t call_count[22];    // Per-function call counts
    uint64_t total_calls;        // Total calls to all functions
    uint64_t intercepted_calls;  // Calls routed through backend
    uint64_t fallback_calls;     // Calls to original functions
} tty_hook_stats_t;

// Get current hook statistics
void tty_hooks_get_stats(tty_hook_stats_t* stats);

// Reset hook statistics
void tty_hooks_reset_stats(void);

// Get function name by index (0-21)
const char* tty_hooks_get_function_name(int index);

// Get total number of hooked functions
int tty_hooks_get_count(void);

// ============================================================================
// Intercepted Functions (22 total)
// ============================================================================

// These declarations allow the hook implementations to be linked
// in place of the original tmux functions

// Character/cell operations
void tty_cmd_cell(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_cells(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_insertcharacter(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_deletecharacter(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_clearcharacter(struct tty* tty, const struct tty_ctx* ctx);

// Line operations
void tty_cmd_insertline(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_deleteline(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_clearline(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_clearendofline(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_clearstartofline(struct tty* tty, const struct tty_ctx* ctx);

// Screen operations
void tty_cmd_clearscreen(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_clearendofscreen(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_clearstartofscreen(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_alignmenttest(struct tty* tty, const struct tty_ctx* ctx);

// Scrolling operations
void tty_cmd_reverseindex(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_linefeed(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_scrollup(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_scrolldown(struct tty* tty, const struct tty_ctx* ctx);

// Special operations
void tty_cmd_setselection(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_rawstring(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_sixelimage(struct tty* tty, const struct tty_ctx* ctx);
void tty_cmd_syncstart(struct tty* tty, const struct tty_ctx* ctx);

#endif // TTY_WRITE_HOOKS_H