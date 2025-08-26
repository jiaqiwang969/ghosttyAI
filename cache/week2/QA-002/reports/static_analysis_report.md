# Static Analysis Report
**Date**: 2025-08-26
**Task**: T-404 Defect Fixes

## Analysis Summary

## Event Loop Backend (T-201)

### event_loop_router.c

#### Potential Issues:
- **P2**: Potential integer overflow in calculations

### event_loop_backend.h

#### Potential Issues:
- **P2**: Potential integer overflow in calculations

## Grid Operations (T-202)

### grid_batch_ops.c

#### Potential Issues:
- **P2**: Potential integer overflow in calculations

### dirty_tracking.c

#### Potential Issues:
- **P2**: Potential integer overflow in calculations

## FFI Types (T-301)

### c_types.zig

#### Potential Issues:
- **P1**: Unsafe pointer operations detected
163:        return @ptrCast(*const UiCell, c_ptr);
167:        return @ptrCast(*const c.ui_cell_t, self);
587:    const backend_struct = @ptrCast(*UiBackendStruct, backend);

- **P2**: Try without proper error handling

## Ghostty Integration (T-302)

### callbacks.zig

#### Potential Issues:
- **P1**: Unsafe pointer operations detected
133:    const backend_struct = @ptrCast(*c_types.UiBackendStruct, backend);
134:    return @ptrCast(*GhosttyBackendState, @alignCast(@alignOf(GhosttyBackendState), backend_struct.priv));
827:    const backend_struct = @ptrCast(*c_types.UiBackendStruct, backend);


### c_types.zig

#### Potential Issues:
- **P1**: Unsafe pointer operations detected
163:        return @ptrCast(*const UiCell, c_ptr);
167:        return @ptrCast(*const c.ui_cell_t, self);
587:    const backend_struct = @ptrCast(*UiBackendStruct, backend);

- **P2**: Try without proper error handling

### ghostty_backend.zig

#### Potential Issues:
- **P1**: Unsafe pointer operations detected
212:            @ptrCast(*Terminal, terminal),
293:        callbacks.registerCallbacks(@ptrCast(*c_types.UiBackend, &backend.backend));
458:        const self = @ptrCast(*GhosttyBackend, @alignCast(@alignOf(GhosttyBackend), user_data));


### integration.zig

#### Potential Issues:
- **P1**: Unsafe pointer operations detected
162:            @ptrCast(*c_types.UiBackend, &self.backend.backend),
193:        const result = c_types.tty_hooks_install(@ptrCast(*c_types.UiBackend, &self.backend.backend));
219:        const backend_struct = @ptrCast(*c_types.UiBackendStruct, &self.backend.backend);


## Layout Manager (T-203)

### layout_callbacks.h

#### Potential Issues:
- **P0**: Unsafe string functions detected
7:// Performance Targets: <50ms layout change, <10ms pane split, <5ms resize

- **P2**: Potential integer overflow in calculations

### test_layout.c

#### Potential Issues:
- **P2**: Potential integer overflow in calculations

### pane_operations.c

#### Potential Issues:
- **P1**: Potential memory leak (malloc:3, free:2)
- **P2**: Potential integer overflow in calculations

### layout_manager.c

#### Potential Issues:
- **P2**: Potential integer overflow in calculations

## Copy Mode (T-204)

### copy_mode_callbacks.h

#### Potential Issues:
- **P2**: Potential integer overflow in calculations

### test_copy_mode.c

#### Potential Issues:
- **P0**: Unsafe string functions detected
223:    strcpy(grid->lines[5], "Hello World Test String");
253:    strcpy(grid->lines[5], "This is a test string with test word");
254:    strcpy(grid->lines[10], "Another line with test content");
466:    strcpy(grid->lines[0], "Hello 世界 🌍 Test");
467:    strcpy(grid->lines[1], "Emoji: 😀😁😂");

- **P2**: Potential integer overflow in calculations

### copy_mode_backend.c

#### Potential Issues:
- **P2**: Potential integer overflow in calculations

### selection_renderer.c

#### Potential Issues:
- **P1**: Potential memory leak (malloc:10, free:3)
- **P2**: Potential integer overflow in calculations

### clipboard_integration.c

#### Potential Issues:
- **P2**: Potential integer overflow in calculations


## Analysis Complete
Time: Tue Aug 26 15:06:58 CST 2025
