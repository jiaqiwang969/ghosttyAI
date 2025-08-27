# MIGRATION GUIDE: TTY Hooks Interface Update

**Date**: 2025-08-25  
**Version**: 1.1.0  
**Defect**: DEFECT-002  
**Author**: CORE-001 (c-tmux-specialist)  

## Overview

The TTY hooks interface has been standardized to use consistent naming conventions. The primary change is:
- ❌ OLD: `tty_write_hooks_*` functions
- ✅ NEW: `tty_hooks_*` functions

## Changes Summary

| Old Name | New Name | Status |
|----------|----------|--------|
| `tty_write_hooks_init()` | `tty_hooks_init()` | ✅ Updated |
| `tty_write_hooks_install()` | `tty_hooks_install()` | ✅ Updated |
| `tty_write_hooks_uninstall()` | `tty_hooks_uninstall()` | ✅ Updated |
| `tty_write_hooks_get_stats()` | `tty_hooks_get_stats()` | ✅ Updated |
| `tty_write_hooks_reset_stats()` | `tty_hooks_reset_stats()` | ✅ Updated |

## Backward Compatibility

**✅ Full backward compatibility is maintained** through macro definitions in `tty_write_hooks.h`.

### Using Compatibility Mode (Default)
```c
// Old code continues to work without changes
tty_write_hooks_init();  // Automatically mapped to tty_hooks_init()
```

### Using New Interface (Recommended)
```c
// New standardized interface
tty_hooks_init();
```

### Disabling Compatibility Macros
```c
#define TTY_HOOKS_NO_COMPAT  // Before including the header
#include "tty_write_hooks.h"
// Now only new names are available
```

### Enabling Deprecation Warnings
```c
#define TTY_HOOKS_WARN_DEPRECATED  // Before including the header
#include "tty_write_hooks.h"
// Compiler will warn about old names
```

## Migration Steps for Teams

### For CORE-002 (libtmux-core-developer)
✅ **No changes required** - backend_router.c already expects the standardized names

### For INTG-001 (zig-ghostty-integration)
```zig
// Update Zig FFI bindings
extern fn tty_hooks_init() void;  // Use new name
extern fn tty_hooks_install(backend: *ui_backend) c_int;
extern fn tty_hooks_uninstall() c_int;
```

### For QA Teams
```c
// Update test files to use new names
void test_hooks() {
    tty_hooks_init();  // New name
    // ... rest of test
}
```

## Integration with backend_router.c

The fixed interface now properly aligns with CORE-002's backend_router.c:

```c
// backend_router.c expects:
#include "tty_write_hooks.h"

// And calls:
tty_hooks_init();
tty_hooks_install(backend);
tty_hooks_uninstall();
```

## Testing the Migration

### Verify Compatibility
```bash
cd cache/week1/CORE-001/fixed
gcc -o test_compat test_compatibility.c tty_write_hooks.c -I../ARCH-001
./test_compat
```

### Run Full Test Suite
```bash
cd cache/week1/CORE-001/fixed
make test
```

## Timeline

- **Immediate**: Fixed version available in `cache/week1/CORE-001/fixed/`
- **Transition Period**: 2 weeks with compatibility macros
- **Final**: Remove compatibility macros after all teams migrate

## Benefits of Standardization

1. **Consistency**: All hook functions use `tty_hooks_*` prefix
2. **Clarity**: Clearer that these are general hooks, not just write hooks
3. **Integration**: Matches expectations of backend_router.c
4. **Future-proof**: Easier to add new hook types

## Rollback Plan

If issues arise:
1. Original version remains in `cache/week1/CORE-001/`
2. Compatibility macros ensure no breaking changes
3. Can revert by copying original files back

## Questions/Support

**Contact**: CORE-001 (c-tmux-specialist)  
**Session**: ghostty-core:0  
**Priority**: P0 - Integration blocker resolved

---

**Status**: ✅ Ready for integration