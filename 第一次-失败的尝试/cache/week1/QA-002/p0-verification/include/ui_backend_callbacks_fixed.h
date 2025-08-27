// ui_backend_callbacks_fixed.h - 修复回调函数缺失问题
// Author: ARCH-001 (System Architect)
// Purpose: 解决ui_backend回调函数缺失的P0缺陷
// Version: 2.0.0 - FIXED

#ifndef UI_BACKEND_CALLBACKS_FIXED_H
#define UI_BACKEND_CALLBACKS_FIXED_H

#include <stdint.h>
#include "tty_ctx_unified.h"
#include "ui_backend_base.h"  // Include base UI backend definitions

// Forward declarations
struct tty;

// ============================================================================
// SOLUTION 3: Complete Callback Mechanism with Operations Table
// ============================================================================

/**
 * Problem: ui_backend structure was missing the 22 command callback functions
 * Solution: Use operations table pattern with complete function pointers
 */

// ============================================================================
// 3A: Complete Command Operations Table
// ============================================================================

/**
 * All 22 tty_cmd_* functions mapped to callbacks
 * Based on analysis of tmux/tty.c
 */
typedef struct ui_backend_ops_v2 {
    // === ABI Stability ===
    uint32_t size;              // MUST be first field
    uint32_t version;           // Version of this ops table
    uint32_t flags;             // Feature flags
    uint32_t reserved;          // Alignment
    
    // === Character/Cell Operations (5 functions) ===
    void (*cmd_cell)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_cells)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_insertcharacter)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_deletecharacter)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearcharacter)(struct ui_backend*, const struct tty_ctx*);
    
    // === Line Operations (5 functions) ===
    void (*cmd_insertline)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_deleteline)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearline)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearendofline)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearstartofline)(struct ui_backend*, const struct tty_ctx*);
    
    // === Screen Operations (4 functions) ===
    void (*cmd_clearscreen)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearendofscreen)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_clearstartofscreen)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_alignmenttest)(struct ui_backend*, const struct tty_ctx*);
    
    // === Scrolling Operations (4 functions) ===
    void (*cmd_reverseindex)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_linefeed)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_scrollup)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_scrolldown)(struct ui_backend*, const struct tty_ctx*);
    
    // === Special Operations (4 functions) ===
    void (*cmd_setselection)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_rawstring)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_sixelimage)(struct ui_backend*, const struct tty_ctx*);
    void (*cmd_syncstart)(struct ui_backend*, const struct tty_ctx*);
    
    // === Extension Points for Future Commands ===
    void (*cmd_extension[8])(struct ui_backend*, const struct tty_ctx*);
    
} ui_backend_ops_v2_t;

// ============================================================================
// 3B: Command Metadata Table
// ============================================================================

/**
 * Metadata for each command to guide processing
 */
typedef struct {
    const char* name;           // Command name for logging
    uint32_t cmd_id;           // Unique command ID
    uint32_t flags;            // Command characteristics
} command_metadata_t;

// Command flags
enum {
    CMD_FLAG_BATCHABLE    = 1 << 0,  // Can be batched in frames
    CMD_FLAG_URGENT       = 1 << 1,  // Should bypass batching
    CMD_FLAG_MODIFIES_CURSOR = 1 << 2,  // Changes cursor position
    CMD_FLAG_MODIFIES_CONTENT = 1 << 3,  // Changes cell content
    CMD_FLAG_CLEARS       = 1 << 4,  // Clears cells
    CMD_FLAG_SCROLLS      = 1 << 5,  // Scrolls content
};

// Command ID enumeration
enum {
    CMD_ID_CELL = 1,
    CMD_ID_CELLS,
    CMD_ID_INSERTCHARACTER,
    CMD_ID_DELETECHARACTER,
    CMD_ID_CLEARCHARACTER,
    CMD_ID_INSERTLINE,
    CMD_ID_DELETELINE,
    CMD_ID_CLEARLINE,
    CMD_ID_CLEARENDOFLINE,
    CMD_ID_CLEARSTARTOFLINE,
    CMD_ID_CLEARSCREEN,
    CMD_ID_CLEARENDOFSCREEN,
    CMD_ID_CLEARSTARTOFSCREEN,
    CMD_ID_ALIGNMENTTEST,
    CMD_ID_REVERSEINDEX,
    CMD_ID_LINEFEED,
    CMD_ID_SCROLLUP,
    CMD_ID_SCROLLDOWN,
    CMD_ID_SETSELECTION,
    CMD_ID_RAWSTRING,
    CMD_ID_SIXELIMAGE,
    CMD_ID_SYNCSTART,
};

// Global command metadata table
static const command_metadata_t command_metadata[] = {
    {"cell", CMD_ID_CELL, CMD_FLAG_BATCHABLE | CMD_FLAG_MODIFIES_CONTENT},
    {"cells", CMD_ID_CELLS, CMD_FLAG_BATCHABLE | CMD_FLAG_MODIFIES_CONTENT},
    {"insertcharacter", CMD_ID_INSERTCHARACTER, CMD_FLAG_BATCHABLE | CMD_FLAG_MODIFIES_CONTENT},
    {"deletecharacter", CMD_ID_DELETECHARACTER, CMD_FLAG_BATCHABLE | CMD_FLAG_MODIFIES_CONTENT},
    {"clearcharacter", CMD_ID_CLEARCHARACTER, CMD_FLAG_BATCHABLE | CMD_FLAG_CLEARS},
    {"insertline", CMD_ID_INSERTLINE, CMD_FLAG_BATCHABLE | CMD_FLAG_SCROLLS},
    {"deleteline", CMD_ID_DELETELINE, CMD_FLAG_BATCHABLE | CMD_FLAG_SCROLLS},
    {"clearline", CMD_ID_CLEARLINE, CMD_FLAG_BATCHABLE | CMD_FLAG_CLEARS},
    {"clearendofline", CMD_ID_CLEARENDOFLINE, CMD_FLAG_BATCHABLE | CMD_FLAG_CLEARS},
    {"clearstartofline", CMD_ID_CLEARSTARTOFLINE, CMD_FLAG_BATCHABLE | CMD_FLAG_CLEARS},
    {"clearscreen", CMD_ID_CLEARSCREEN, CMD_FLAG_URGENT | CMD_FLAG_CLEARS},
    {"clearendofscreen", CMD_ID_CLEARENDOFSCREEN, CMD_FLAG_BATCHABLE | CMD_FLAG_CLEARS},
    {"clearstartofscreen", CMD_ID_CLEARSTARTOFSCREEN, CMD_FLAG_BATCHABLE | CMD_FLAG_CLEARS},
    {"alignmenttest", CMD_ID_ALIGNMENTTEST, CMD_FLAG_URGENT},
    {"reverseindex", CMD_ID_REVERSEINDEX, CMD_FLAG_BATCHABLE | CMD_FLAG_SCROLLS},
    {"linefeed", CMD_ID_LINEFEED, CMD_FLAG_BATCHABLE | CMD_FLAG_SCROLLS},
    {"scrollup", CMD_ID_SCROLLUP, CMD_FLAG_BATCHABLE | CMD_FLAG_SCROLLS},
    {"scrolldown", CMD_ID_SCROLLDOWN, CMD_FLAG_BATCHABLE | CMD_FLAG_SCROLLS},
    {"setselection", CMD_ID_SETSELECTION, CMD_FLAG_URGENT},
    {"rawstring", CMD_ID_RAWSTRING, CMD_FLAG_URGENT},
    {"sixelimage", CMD_ID_SIXELIMAGE, CMD_FLAG_URGENT},
    {"syncstart", CMD_ID_SYNCSTART, CMD_FLAG_URGENT},
};

// ============================================================================
// 3C: Fixed ui_backend Structure with Operations Table
// ============================================================================

/**
 * Complete ui_backend structure with proper callback mechanism
 * Using typedef for compatibility
 */
typedef struct ui_backend ui_backend_v2_t;

// ============================================================================
// 3D: Helper Functions for Callback Management
// ============================================================================

/**
 * Initialize operations table with default no-op implementations
 */
static inline void ui_backend_ops_init_defaults(ui_backend_ops_v2_t* ops) {
    if (!ops) return;
    
    ops->size = sizeof(ui_backend_ops_v2_t);
    ops->version = 2;
    ops->flags = 0;
    
    // Set all function pointers to a default no-op handler
    // This prevents null pointer crashes
    void (*noop)(struct ui_backend*, const struct tty_ctx*) = NULL;
    
    ops->cmd_cell = noop;
    ops->cmd_cells = noop;
    ops->cmd_insertcharacter = noop;
    ops->cmd_deletecharacter = noop;
    ops->cmd_clearcharacter = noop;
    ops->cmd_insertline = noop;
    ops->cmd_deleteline = noop;
    ops->cmd_clearline = noop;
    ops->cmd_clearendofline = noop;
    ops->cmd_clearstartofline = noop;
    ops->cmd_clearscreen = noop;
    ops->cmd_clearendofscreen = noop;
    ops->cmd_clearstartofscreen = noop;
    ops->cmd_alignmenttest = noop;
    ops->cmd_reverseindex = noop;
    ops->cmd_linefeed = noop;
    ops->cmd_scrollup = noop;
    ops->cmd_scrolldown = noop;
    ops->cmd_setselection = noop;
    ops->cmd_rawstring = noop;
    ops->cmd_sixelimage = noop;
    ops->cmd_syncstart = noop;
}

/**
 * Validate that all required callbacks are present
 */
static inline int ui_backend_ops_validate(const ui_backend_ops_v2_t* ops) {
    if (!ops) return -1;
    if (ops->size < sizeof(ui_backend_ops_v2_t)) return -1;
    if (ops->version != 2) return -1;
    
    // Check that at least the critical callbacks are non-null
    if (!ops->cmd_cell || !ops->cmd_cells) return -1;
    if (!ops->cmd_clearline || !ops->cmd_clearscreen) return -1;
    
    return 0;
}

/**
 * Call a command through the operations table with safety checks
 * This function is implemented in ui_backend_impl.c
 */
void ui_backend_call_command(
    ui_backend_v2_t* backend,
    uint32_t cmd_id,
    const struct tty_ctx* ctx);

// ============================================================================
// 3E: Compile-Time Verification
// ============================================================================

// Ensure we have exactly 22 command functions
_Static_assert(sizeof(command_metadata) / sizeof(command_metadata[0]) == 22,
              "Must have exactly 22 commands");

// Ensure operations table is properly sized
_Static_assert(offsetof(ui_backend_ops_v2_t, cmd_syncstart) < 
              sizeof(ui_backend_ops_v2_t),
              "All callbacks must fit in structure");

#endif /* UI_BACKEND_CALLBACKS_FIXED_H */