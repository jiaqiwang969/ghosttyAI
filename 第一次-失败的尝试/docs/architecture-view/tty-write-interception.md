# tty_write Interception Architecture

## Overview
This document details the precise modification points in tmux for intercepting the tty_write chain and routing to the UI backend system.

## Current tty_write Flow

### Call Chain
```
screen_write_* functions (screen-write.c)
    ↓
tty_write(tty_cmd_function, &ttyctx)
    ↓
tty_cmd_* functions (tty.c)
    ↓
Terminal escape sequences via tty_puts/tty_putc
```

## Key tty_cmd Functions to Intercept

### Cell Drawing Commands
```c
// Primary cell output - highest frequency
tty_cmd_cell(struct tty *tty, const struct tty_ctx *ctx)
tty_cmd_cells(struct tty *tty, const struct tty_ctx *ctx)

// Clear operations
tty_cmd_clearcharacter(struct tty *tty, const struct tty_ctx *ctx)
tty_cmd_clearline(struct tty *tty, const struct tty_ctx *ctx)
tty_cmd_clearendofline(struct tty *tty, const struct tty_ctx *ctx)
tty_cmd_clearstartofline(struct tty *tty, const struct tty_ctx *ctx)
tty_cmd_clearscreen(struct tty *tty, const struct tty_ctx *ctx)
tty_cmd_clearendofscreen(struct tty *tty, const struct tty_ctx *ctx)
tty_cmd_clearstartofscreen(struct tty *tty, const struct tty_ctx *ctx)
```

### Line Operations
```c
tty_cmd_insertline(struct tty *tty, const struct tty_ctx *ctx)
tty_cmd_deleteline(struct tty *tty, const struct tty_ctx *ctx)
tty_cmd_reverseindex(struct tty *tty, const struct tty_ctx *ctx)
tty_cmd_linefeed(struct tty *tty, const struct tty_ctx *ctx)
```

### Character Operations
```c
tty_cmd_insertcharacter(struct tty *tty, const struct tty_ctx *ctx)
tty_cmd_deletecharacter(struct tty *tty, const struct tty_ctx *ctx)
```

### Scrolling
```c
tty_cmd_scrollup(struct tty *tty, const struct tty_ctx *ctx)
tty_cmd_scrolldown(struct tty *tty, const struct tty_ctx *ctx)
```

### Special Operations
```c
tty_cmd_alignmenttest(struct tty *tty, const struct tty_ctx *ctx)
tty_cmd_setselection(struct tty *tty, const struct tty_ctx *ctx)
tty_cmd_rawstring(struct tty *tty, const struct tty_ctx *ctx)
tty_cmd_sixelimage(struct tty *tty, const struct tty_ctx *ctx)
tty_cmd_syncstart(struct tty *tty, const struct tty_ctx *ctx)
```

## Modification Strategy

### Step 1: Add Backend Selection to tty Structure
```c
// In tmux.h, modify struct tty:
struct tty {
    // ... existing fields ...
    
    struct ui_backend *backend;  // NEW: UI backend if not using TTY
    int backend_mode;            // NEW: 0=TTY, 1=UI_BACKEND
};
```

### Step 2: Create Backend Dispatcher
```c
// In tty.c, add backend dispatcher:
void
tty_write(void (*cmdfn)(struct tty *, const struct tty_ctx *),
    struct tty_ctx *ctx)
{
    struct tty *tty = ctx->tty;
    
    if (tty->backend_mode == 1 && tty->backend != NULL) {
        // Route to UI backend
        ui_backend_write(tty->backend, cmdfn, ctx);
        return;
    }
    
    // Original TTY path
    cmdfn(tty, ctx);
}
```

### Step 3: Implement UI Backend Router
```c
// In ui_backend.c (new file):
void
ui_backend_write(struct ui_backend *backend, 
    void (*cmdfn)(struct tty *, const struct tty_ctx *),
    struct tty_ctx *ctx)
{
    // Map function pointer to operation type
    if (cmdfn == tty_cmd_cell) {
        backend_handle_cell(backend, ctx);
    } else if (cmdfn == tty_cmd_cells) {
        backend_handle_cells(backend, ctx);
    } else if (cmdfn == tty_cmd_clearline) {
        backend_handle_clear_line(backend, ctx);
    }
    // ... handle all commands
}
```

### Step 4: Grid Update Aggregation
```c
// Aggregate cell updates for efficiency
struct grid_aggregator {
    struct tmc_span spans[MAX_SPANS];
    u_int span_count;
    u_int current_row;
    u_int current_col;
    struct tmc_cell cell_buffer[MAX_CELLS];
    u_int cell_count;
};

static void
backend_handle_cell(struct ui_backend *backend, struct tty_ctx *ctx)
{
    struct grid_aggregator *agg = backend->aggregator;
    struct grid_cell *gc = ctx->cell;
    
    // Convert tmux cell to tmc_cell
    struct tmc_cell cell = {
        .ch = gc->data.data[0],  // Simplified, handle UTF-8 properly
        .fg_rgb = colour_to_rgb(gc->fg),
        .bg_rgb = colour_to_rgb(gc->bg),
        .attrs = gc->attr,
        .width = gc->data.width
    };
    
    // Add to current span or start new one
    if (should_start_new_span(agg, ctx)) {
        flush_current_span(agg, backend);
        start_new_span(agg, ctx);
    }
    
    add_cell_to_span(agg, &cell);
}
```

### Step 5: Callback Dispatch
```c
static void
flush_aggregated_updates(struct ui_backend *backend)
{
    struct grid_aggregator *agg = backend->aggregator;
    
    if (agg->span_count == 0)
        return;
    
    struct tmc_grid_update update = {
        .pane_id = backend->current_pane_id,
        .rows = backend->rows,
        .cols = backend->cols,
        .span_count = agg->span_count,
        .spans = agg->spans,
        .full = 0
    };
    
    // Dispatch callback
    if (backend->vtable->on_grid) {
        backend->vtable->on_grid(backend->server, &update, backend->user_data);
    }
    
    // Reset aggregator
    reset_aggregator(agg);
}
```

## Data Structure Mappings

### tmux grid_cell → tmc_cell
```c
// Color conversion
uint32_t colour_to_rgb(int colour) {
    if (colour & COLOUR_FLAG_RGB) {
        return colour & 0xffffff;
    } else if (colour & COLOUR_FLAG_256) {
        return palette_256_to_rgb(colour & 0xff);
    } else {
        return palette_16_to_rgb(colour & 0xf);
    }
}

// Attribute mapping
uint16_t attrs_to_tmc(int attr) {
    uint16_t result = 0;
    if (attr & GRID_ATTR_BRIGHT) result |= TMC_ATTR_BOLD;
    if (attr & GRID_ATTR_DIM) result |= TMC_ATTR_DIM;
    if (attr & GRID_ATTR_UNDERSCORE) result |= TMC_ATTR_UNDER;
    if (attr & GRID_ATTR_BLINK) result |= TMC_ATTR_BLINK;
    if (attr & GRID_ATTR_REVERSE) result |= TMC_ATTR_REVERSE;
    if (attr & GRID_ATTR_HIDDEN) result |= TMC_ATTR_HIDDEN;
    if (attr & GRID_ATTR_ITALICS) result |= TMC_ATTR_ITALIC;
    if (attr & GRID_ATTR_STRIKETHROUGH) result |= TMC_ATTR_STRIKE;
    return result;
}
```

## Optimization Strategies

### 1. Span Coalescing
Combine adjacent cells with same attributes into single span:
```c
static int
can_coalesce(struct tmc_cell *last, struct tmc_cell *new) {
    return (last->fg_rgb == new->fg_rgb &&
            last->bg_rgb == new->bg_rgb &&
            last->attrs == new->attrs);
}
```

### 2. Dirty Rectangle Tracking
Only send updates for modified regions:
```c
struct dirty_rect {
    u_int x1, y1, x2, y2;
    int valid;
};

static void
mark_dirty(struct ui_backend *backend, u_int x, u_int y) {
    struct dirty_rect *dr = &backend->dirty;
    if (!dr->valid) {
        dr->x1 = dr->x2 = x;
        dr->y1 = dr->y2 = y;
        dr->valid = 1;
    } else {
        if (x < dr->x1) dr->x1 = x;
        if (x > dr->x2) dr->x2 = x;
        if (y < dr->y1) dr->y1 = y;
        if (y > dr->y2) dr->y2 = y;
    }
}
```

### 3. Frame-based Batching
Aggregate updates for 16.67ms (60 FPS):
```c
static void
schedule_flush(struct ui_backend *backend) {
    if (backend->flush_pending)
        return;
    
    struct timeval tv = {
        .tv_sec = 0,
        .tv_usec = 16667  // ~60 FPS
    };
    
    evtimer_add(&backend->flush_timer, &tv);
    backend->flush_pending = 1;
}
```

## Thread Safety Considerations

### Callback Synchronization
```c
// If callbacks cross thread boundaries
static void
thread_safe_callback(struct ui_backend *backend, 
                    struct tmc_grid_update *update)
{
    // Option 1: Copy data
    struct tmc_grid_update *copy = copy_grid_update(update);
    post_to_ui_thread(backend, copy);
    
    // Option 2: Lock-free queue
    enqueue_update(backend->update_queue, update);
    signal_ui_thread(backend);
}
```

## Testing Points

### Unit Tests
1. Cell conversion accuracy
2. Span coalescing logic
3. Dirty rectangle calculation
4. Attribute mapping
5. Color space conversion

### Integration Tests
1. Full screen redraw
2. Scrolling performance
3. Split pane updates
4. Copy mode rendering
5. Unicode handling

### Performance Tests
1. Callback overhead measurement
2. Aggregation efficiency
3. Memory allocation patterns
4. Update frequency analysis