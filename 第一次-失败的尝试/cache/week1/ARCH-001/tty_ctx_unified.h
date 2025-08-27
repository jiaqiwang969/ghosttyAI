// tty_ctx_unified.h - 统一的tty_ctx定义
// Author: ARCH-001 (System Architect)
// Purpose: 解决struct tty_ctx定义不一致的P0缺陷
// Version: 2.0.0 - FIXED

#ifndef TTY_CTX_UNIFIED_H
#define TTY_CTX_UNIFIED_H

#include <stdint.h>
#include <stddef.h>

// Forward declarations to avoid circular dependencies
struct screen;
struct window_pane;
struct grid_cell;
struct tty;

// ============================================================================
// SOLUTION 1: Unified tty_ctx Definition with ABI Stability
// ============================================================================

/**
 * Unified tty_ctx structure that matches tmux internal definition
 * This MUST be kept in sync with tmux/tmux.h
 */
typedef struct tty_ctx {
    // === ABI Stability Header ===
    uint32_t size;              // MUST be first field for ABI stability
    uint32_t version;           // Version of this structure (currently 1)
    
    // === Core tmux fields (from tmux.h) ===
    struct screen *s;           // Screen reference
    
    // Callback function pointers (if needed by tmux)
    void (*redraw_cb)(void*);
    void (*set_client_cb)(void*);
    void *arg;                  // Callback argument
    
    // Cell data
    const struct grid_cell *cell;  // Current cell being processed
    int wrapped;                    // Line wrap flag
    
    // Generic parameters
    uint32_t num;               // Number parameter (cells, lines, etc.)
    void *ptr;                  // Generic pointer 1
    void *ptr2;                 // Generic pointer 2
    
    // Visibility control
    int allow_invisible_panes;  // For passthrough sequences
    
    // === Critical cursor/region position fields ===
    // These were MISSING and caused the P0 defect
    uint32_t ocx;               // Original cursor X position
    uint32_t ocy;               // Original cursor Y position
    uint32_t orupper;           // Original scroll region upper bound
    uint32_t orlower;           // Original scroll region lower bound
    
    // === Additional fields for safety ===
    struct window_pane *wp;     // Window pane reference
    struct tty *tty;            // TTY reference
    
    // === Extension fields for future use ===
    uint32_t flags;             // Extension flags
    uint32_t reserved[8];       // Reserved for future expansion
} tty_ctx_t;

// ============================================================================
// SOLUTION 1A: Compatibility Layer for Different Versions
// ============================================================================

/**
 * Version detection and compatibility handling
 */
#define TTY_CTX_VERSION_1 1
#define TTY_CTX_VERSION_CURRENT TTY_CTX_VERSION_1

/**
 * Initialize tty_ctx with proper size and version
 */
static inline void tty_ctx_init(struct tty_ctx *ctx) {
    if (ctx) {
        ctx->size = sizeof(struct tty_ctx);
        ctx->version = TTY_CTX_VERSION_CURRENT;
        // Initialize critical fields to safe defaults
        ctx->ocx = 0;
        ctx->ocy = 0;
        ctx->orupper = 0;
        ctx->orlower = 0;
    }
}

/**
 * Check if a tty_ctx has required fields
 */
static inline int tty_ctx_is_valid(const struct tty_ctx *ctx) {
    if (!ctx) return 0;
    if (ctx->size < offsetof(struct tty_ctx, orlower) + sizeof(uint32_t)) {
        return 0; // Missing critical fields
    }
    return 1;
}

// ============================================================================
// SOLUTION 1B: Safe Field Access Macros
// ============================================================================

/**
 * Safe field access that checks structure size before accessing
 */
#define TTY_CTX_GET_FIELD(ctx_ptr, field, default_value) \
    (((ctx_ptr) && (ctx_ptr)->size >= offsetof(struct tty_ctx, field) + sizeof(((struct tty_ctx*)0)->field)) \
     ? (ctx_ptr)->field : (default_value))

#define TTY_CTX_SET_FIELD(ctx_ptr, field, value) do { \
    if ((ctx_ptr) && (ctx_ptr)->size >= offsetof(struct tty_ctx, field) + sizeof(((struct tty_ctx*)0)->field)) { \
        (ctx_ptr)->field = (value); \
    } \
} while(0)

// Convenience macros for critical fields  
#define TTY_CTX_GET_OCX(ctx_ptr) TTY_CTX_GET_FIELD(ctx_ptr, ocx, 0)
#define TTY_CTX_GET_OCY(ctx_ptr) TTY_CTX_GET_FIELD(ctx_ptr, ocy, 0)
#define TTY_CTX_GET_CELL(ctx_ptr) TTY_CTX_GET_FIELD(ctx_ptr, cell, NULL)

// ============================================================================
// SOLUTION 1C: Structure Validation and Migration
// ============================================================================

/**
 * Migrate old tty_ctx to new format
 * Returns 0 on success, -1 on failure
 */
static inline int tty_ctx_migrate(struct tty_ctx *ctx) {
    if (!ctx) return -1;
    
    // Check if already current version
    if (ctx->size == sizeof(struct tty_ctx) && 
        ctx->version == TTY_CTX_VERSION_CURRENT) {
        return 0; // Already migrated
    }
    
    // Save old data
    struct tty_ctx old = *ctx;
    
    // Re-initialize with new size
    tty_ctx_init(ctx);
    
    // Copy over common fields
    ctx->s = old.s;
    ctx->cell = old.cell;
    ctx->num = old.num;
    ctx->ptr = old.ptr;
    ctx->ptr2 = old.ptr2;
    
    // Set defaults for new fields
    ctx->ocx = 0;
    ctx->ocy = 0;
    ctx->orupper = 0;
    ctx->orlower = 0;
    
    return 0;
}

// ============================================================================
// SOLUTION 1D: Compile-Time Assertions for Safety
// ============================================================================

// Ensure structure is properly aligned and sized
_Static_assert(offsetof(struct tty_ctx, size) == 0, 
              "size must be first field");
_Static_assert(sizeof(struct tty_ctx) % 8 == 0, 
              "tty_ctx must be 8-byte aligned");

// Ensure critical fields are at expected offsets
_Static_assert(offsetof(struct tty_ctx, ocx) < sizeof(struct tty_ctx),
              "ocx must be within structure");
_Static_assert(offsetof(struct tty_ctx, ocy) < sizeof(struct tty_ctx),
              "ocy must be within structure");

#endif /* TTY_CTX_UNIFIED_H */