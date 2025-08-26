# Week 5 Critical Implementation Guide - screen-write.c Modifications

## 🎯 Most Critical Task: Adding Command IDs to screen-write.c

This document provides the EXACT changes needed for `/Users/jqwang/98-ghosttyAI/tmux/screen-write.c` to make the entire integration work.

## 📍 Step 1: Add Command ID Definitions

Add this at the top of screen-write.c after includes:

```c
#ifdef LIBTMUXCORE_BUILD
/* Command IDs for UI Backend dispatch */
enum {
    TTY_CMD_UNKNOWN = 0,
    TTY_CMD_CELL = 1,
    TTY_CMD_CLEARLINE = 2,
    TTY_CMD_CLEARSCREEN = 3,
    TTY_CMD_INSERTLINE = 4,
    TTY_CMD_DELETELINE = 5,
    TTY_CMD_CLEARENDOFLINE = 6,
    TTY_CMD_REVERSEINDEX = 7,
    TTY_CMD_LINEFEED = 8,
    TTY_CMD_SCROLLUP = 9,
    TTY_CMD_SCROLLDOWN = 10
};
#endif
```

## 📍 Step 2: Modify Each tty_write Call Site

### Pattern to Follow:
```c
// BEFORE:
tty_write(tty_cmd_cell, &ttyctx);

// AFTER:
#ifdef LIBTMUXCORE_BUILD
ttyctx.ui_cmd_id = TTY_CMD_CELL;
#endif
tty_write(tty_cmd_cell, &ttyctx);
```

## 📍 Step 3: Specific Function Modifications

### 1. screen_write_cell() - Line ~1730
```c
void screen_write_cell(struct screen_write_ctx *ctx, const struct grid_cell *gc)
{
    // ... existing code ...
    
    // Around line 1850 - before tty_write call:
    #ifdef LIBTMUXCORE_BUILD
    ttyctx.ui_cmd_id = TTY_CMD_CELL;
    #endif
    tty_write(tty_cmd_cell, &ttyctx);
}
```

### 2. screen_write_clearline() - Line ~1100
```c
void screen_write_clearline(struct screen_write_ctx *ctx, u_int bg)
{
    // ... existing code ...
    
    // Before tty_write:
    #ifdef LIBTMUXCORE_BUILD
    ttyctx.ui_cmd_id = TTY_CMD_CLEARLINE;
    #endif
    tty_write(tty_cmd_clearline, &ttyctx);
}
```

### 3. screen_write_clearscreen() - Line ~1050
```c
void screen_write_clearscreen(struct screen_write_ctx *ctx, u_int bg)
{
    // ... existing code ...
    
    #ifdef LIBTMUXCORE_BUILD
    ttyctx.ui_cmd_id = TTY_CMD_CLEARSCREEN;
    #endif
    tty_write(tty_cmd_clearscreen, &ttyctx);
}
```

### 4. screen_write_insertline() - Line ~850
```c
void screen_write_insertline(struct screen_write_ctx *ctx, u_int ny, u_int bg)
{
    // ... existing code ...
    
    #ifdef LIBTMUXCORE_BUILD
    ttyctx.ui_cmd_id = TTY_CMD_INSERTLINE;
    #endif
    tty_write(tty_cmd_insertline, &ttyctx);
}
```

### 5. screen_write_deleteline() - Line ~900
```c
void screen_write_deleteline(struct screen_write_ctx *ctx, u_int ny, u_int bg)
{
    // ... existing code ...
    
    #ifdef LIBTMUXCORE_BUILD
    ttyctx.ui_cmd_id = TTY_CMD_DELETELINE;
    #endif
    tty_write(tty_cmd_deleteline, &ttyctx);
}
```

### 6. screen_write_clearendofline() - Line ~1150
```c
void screen_write_clearendofline(struct screen_write_ctx *ctx, u_int bg)
{
    // ... existing code ...
    
    #ifdef LIBTMUXCORE_BUILD
    ttyctx.ui_cmd_id = TTY_CMD_CLEARENDOFLINE;
    #endif
    tty_write(tty_cmd_clearendofline, &ttyctx);
}
```

### 7. screen_write_reverseindex() - Line ~700
```c
void screen_write_reverseindex(struct screen_write_ctx *ctx, u_int bg)
{
    // ... existing code ...
    
    #ifdef LIBTMUXCORE_BUILD
    ttyctx.ui_cmd_id = TTY_CMD_REVERSEINDEX;
    #endif
    tty_write(tty_cmd_reverseindex, &ttyctx);
}
```

### 8. screen_write_linefeed() - Line ~650
```c
void screen_write_linefeed(struct screen_write_ctx *ctx, int wrapped, u_int bg)
{
    // ... existing code ...
    
    #ifdef LIBTMUXCORE_BUILD
    ttyctx.ui_cmd_id = TTY_CMD_LINEFEED;
    #endif
    tty_write(tty_cmd_linefeed, &ttyctx);
}
```

### 9. screen_write_scrollup() - Line ~550
```c
void screen_write_scrollup(struct screen_write_ctx *ctx, u_int ny, u_int bg)
{
    // ... existing code ...
    
    #ifdef LIBTMUXCORE_BUILD
    ttyctx.ui_cmd_id = TTY_CMD_SCROLLUP;
    #endif
    tty_write(tty_cmd_scrollup, &ttyctx);
}
```

### 10. screen_write_scrolldown() - Line ~600
```c
void screen_write_scrolldown(struct screen_write_ctx *ctx, u_int ny, u_int bg)
{
    // ... existing code ...
    
    #ifdef LIBTMUXCORE_BUILD
    ttyctx.ui_cmd_id = TTY_CMD_SCROLLDOWN;
    #endif
    tty_write(tty_cmd_scrolldown, &ttyctx);
}
```

## 📍 Step 4: Find All tty_write Calls

Use this command to find all locations:
```bash
cd /Users/jqwang/98-ghosttyAI/tmux
grep -n "tty_write(" screen-write.c
```

Expected output (approximate line numbers):
```
658: tty_write(tty_cmd_linefeed, &ttyctx);
710: tty_write(tty_cmd_reverseindex, &ttyctx);
859: tty_write(tty_cmd_insertline, &ttyctx);
912: tty_write(tty_cmd_deleteline, &ttyctx);
1057: tty_write(tty_cmd_clearscreen, &ttyctx);
1108: tty_write(tty_cmd_clearline, &ttyctx);
1158: tty_write(tty_cmd_clearendofline, &ttyctx);
1855: tty_write(tty_cmd_cell, &ttyctx);
... (may be more)
```

## 📍 Step 5: Verification

After modifications, verify with:

```bash
# 1. Check all modifications are wrapped in #ifdef
grep -B1 "tty_write(" screen-write.c | grep -c "ui_cmd_id"
# Should match the number of tty_write calls

# 2. Compile with LIBTMUXCORE_BUILD
gcc -c -DLIBTMUXCORE_BUILD screen-write.c -o screen-write.o

# 3. Compile without (ensure backward compatibility)
gcc -c screen-write.c -o screen-write-normal.o

# Both should compile without errors
```

## 📍 Step 6: Test the Integration

```c
// Quick test file: test_screen_write_ids.c
#include <stdio.h>
#include "tmux.h"

void test_command_ids() {
    struct tty_ctx ctx = {0};
    
    // Simulate screen write
    ctx.ui_cmd_id = TTY_CMD_CELL;
    printf("Cell command ID: %d\n", ctx.ui_cmd_id);
    
    ctx.ui_cmd_id = TTY_CMD_CLEARLINE;
    printf("Clear line command ID: %d\n", ctx.ui_cmd_id);
    
    // These should match our enum values
    assert(ctx.ui_cmd_id == 2);
}
```

## 🚨 Common Pitfalls to Avoid

### ❌ DON'T:
```c
// Don't forget the #ifdef
ttyctx.ui_cmd_id = TTY_CMD_CELL;  // This breaks normal tmux!
tty_write(tty_cmd_cell, &ttyctx);
```

### ✅ DO:
```c
// Always wrap in #ifdef
#ifdef LIBTMUXCORE_BUILD
ttyctx.ui_cmd_id = TTY_CMD_CELL;
#endif
tty_write(tty_cmd_cell, &ttyctx);
```

### ❌ DON'T:
```c
// Don't set after tty_write
tty_write(tty_cmd_cell, &ttyctx);
ttyctx.ui_cmd_id = TTY_CMD_CELL;  // Too late!
```

### ✅ DO:
```c
// Set before tty_write
#ifdef LIBTMUXCORE_BUILD
ttyctx.ui_cmd_id = TTY_CMD_CELL;
#endif
tty_write(tty_cmd_cell, &ttyctx);
```

## 📊 Expected Result

After these modifications:
1. Normal tmux compilation still works (without -DLIBTMUXCORE_BUILD)
2. libtmuxcore compilation includes command IDs
3. Our dispatch function in ui_backend_dispatch.c receives correct IDs
4. Callbacks are triggered with proper command identification
5. "Hello from tmux" appears in Ghostty!

## 🎯 Success Validation

```bash
# After modifications, run:
cd /Users/jqwang/98-ghosttyAI
./build_week5.sh  # New build script for Week 5
./test_minimal_dispatch

# Should see:
# ✅ Cell dispatch successful: received 'H' at (0,0)
# ✅ Clear line dispatch successful
# ✅ ALL TESTS PASSED!
```

## 📝 Commit Message

```bash
git add tmux/screen-write.c
git commit -m "Add UI Backend command IDs to screen-write.c

- Add command ID enum for UI Backend dispatch
- Set ui_cmd_id before each tty_write() call
- Wrap all changes in #ifdef LIBTMUXCORE_BUILD
- Maintain backward compatibility
- Enables tmux-Ghostty callback integration

This allows the UI Backend dispatcher to correctly identify
commands using IDs instead of unreliable function pointers."
```

---

**This is THE critical change that makes everything work!**