# ABI Stability and Evolution Strategy

## Overview
This document defines the Application Binary Interface (ABI) stability guarantees and evolution mechanisms for libtmuxcore, ensuring backward compatibility while allowing controlled evolution.

## Core Principles

### 1. Versioned Structures
All structures passed between host and core MUST include size and version fields:

```c
typedef struct {
    uint32_t size;      // MUST be first field, sizeof(struct)
    uint32_t version;   // ABI version, incremented on breaking changes
    // ... actual fields ...
} tmc_any_struct_t;
```

### 2. Symbol Versioning

#### Linux (ELF)
```c
// libtmuxcore.map
LIBTMUXCORE_1.0 {
    global:
        tmc_server_new;
        tmc_server_free;
        tmc_client_attach;
        tmc_send_keys;
        tmc_send_text;
        tmc_command;
    local:
        *;
};

LIBTMUXCORE_1.1 {
    global:
        tmc_command_async;
        tmc_get_metrics;
} LIBTMUXCORE_1.0;
```

#### macOS (Mach-O)
```c
// Exported symbols list
_tmc_server_new
_tmc_server_free
_tmc_client_attach
_tmc_send_keys
_tmc_send_text
_tmc_command
```

#### Windows (PE)
```c
// libtmuxcore.def
EXPORTS
    tmc_server_new @1
    tmc_server_free @2
    tmc_client_attach @3
    tmc_send_keys @4
    tmc_send_text @5
    tmc_command @6
```

## Version Negotiation Protocol

### Runtime Version Check
```c
// Host queries library version
uint32_t tmc_get_abi_version(void);
uint32_t tmc_get_abi_minimum(void);

// Usage pattern
uint32_t lib_version = tmc_get_abi_version();
if (lib_version < TMC_ABI_MINIMUM_REQUIRED) {
    // Too old, fail
    return -1;
}
if (lib_version > TMC_ABI_CURRENT) {
    // Newer than us, use compatibility mode
    use_compat_mode = 1;
}
```

### Capability Negotiation
```c
typedef struct {
    uint32_t size;
    uint32_t version;
    
    // Bitmask of supported features
    uint64_t core_caps;
    uint64_t ui_caps;
    
    // Feature-specific versions
    uint16_t grid_api_version;
    uint16_t event_api_version;
    uint16_t command_api_version;
    uint16_t reserved;
} tmc_capabilities_t;

// Negotiation flow
tmc_capabilities_t caps = {
    .size = sizeof(tmc_capabilities_t),
    .version = TMC_ABI_VERSION,
    .ui_caps = TMC_CAP_FRAME_BATCH | TMC_CAP_UTF8_LINES | TMC_CAP_24BIT_COLOR,
};

tmc_err_t err = tmc_negotiate_capabilities(server, &caps);
// caps now contains intersection of supported features
```

## Structure Evolution Rules

### Adding Fields (Compatible)
```c
// Version 1.0
typedef struct {
    uint32_t size;
    uint32_t version;
    uint32_t window_id;
    uint32_t pane_id;
} tmc_target_v1_t;

// Version 1.1 - compatible extension
typedef struct {
    uint32_t size;
    uint32_t version;
    uint32_t window_id;
    uint32_t pane_id;
    uint32_t session_id;  // New field at end
} tmc_target_v2_t;

// Consumer code checks size
void process_target(void* target_ptr) {
    tmc_target_v1_t* target = target_ptr;
    if (target->size >= sizeof(tmc_target_v2_t)) {
        // Can safely access session_id
        tmc_target_v2_t* v2 = target_ptr;
        use_session(v2->session_id);
    }
}
```

### Callback Evolution
```c
// Callback structures use size field for evolution
typedef struct {
    uint32_t size;
    void (*on_grid_update)(tmc_client_t*, const tmc_grid_update_t*);
    void (*on_layout_change)(tmc_client_t*, const tmc_layout_t*);
    void (*on_bell)(tmc_client_t*);
    // Version 1.1 additions
    void (*on_frame)(tmc_client_t*, const tmc_frame_t*);  // Optional
} tmc_callbacks_t;

// Core checks presence before calling
if (cb->size >= offsetof(tmc_callbacks_t, on_frame) + sizeof(void*)) {
    if (cb->on_frame) {
        cb->on_frame(client, frame);
    }
}
```

## Memory Alignment and Packing

### Alignment Requirements
```c
// All structures must be naturally aligned
#pragma pack(push, 8)  // 8-byte alignment

typedef struct {
    uint32_t size;
    uint32_t version;
    uint64_t timestamp;   // 8-byte aligned
    void* user_data;      // Pointer-sized alignment
} tmc_event_t;

#pragma pack(pop)

// Compile-time assertions
_Static_assert(sizeof(tmc_event_t) == 24, "Unexpected padding");
_Static_assert(offsetof(tmc_event_t, timestamp) == 8, "Misaligned field");
```

### Platform Compatibility
```c
// Handle different pointer sizes
#ifdef _WIN64
    typedef uint64_t tmc_handle_t;
#elif defined(__LP64__)
    typedef uint64_t tmc_handle_t;
#else
    typedef uint32_t tmc_handle_t;
#endif

// Ensure consistent sizes across platforms
typedef struct {
    uint32_t size;
    uint32_t version;
    tmc_handle_t handle;
    uint32_t flags;
    uint32_t reserved;  // Explicit padding
} tmc_object_t;
```

## Breaking Change Management

### Major Version Bumps
When breaking changes are unavoidable:

1. **Increment major version**: TMC_ABI_VERSION_MAJOR
2. **Maintain parallel implementations**: Keep old API for transition period
3. **Deprecation warnings**: Mark old APIs with deprecation attributes
4. **Migration guide**: Document conversion path

```c
// Deprecation marking
#ifdef __GNUC__
    #define TMC_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
    #define TMC_DEPRECATED __declspec(deprecated)
#else
    #define TMC_DEPRECATED
#endif

TMC_DEPRECATED tmc_err_t tmc_old_function(void);
```

### Compatibility Shims
```c
// Provide compatibility layer for old consumers
tmc_err_t tmc_server_new_v1(tmc_server_t** server, const tmc_config_v1_t* config) {
    // Convert v1 config to v2
    tmc_config_v2_t v2_config = {
        .size = sizeof(tmc_config_v2_t),
        .version = 2,
        // Map v1 fields
    };
    return tmc_server_new(server, &v2_config);
}
```

## Testing ABI Stability

### ABI Compliance Checker
```bash
# Generate ABI dump for version comparison
abi-dumper libtmuxcore.so -o v1.0.dump
abi-dumper libtmuxcore_new.so -o v1.1.dump

# Check compatibility
abi-compliance-checker -l libtmuxcore \
    -old v1.0.dump -new v1.1.dump

# Verify symbol exports
nm -D libtmuxcore.so | grep " T " | cut -d' ' -f3 | sort > symbols.txt
diff symbols_v1.0.txt symbols_v1.1.txt
```

### Size Assertions
```c
// test/abi_test.c
void test_struct_sizes(void) {
    // Verify critical structure sizes haven't changed
    assert(sizeof(tmc_cell_t) == 16);
    assert(sizeof(tmc_span_t) == 32);
    assert(sizeof(tmc_frame_t) == 48);
    
    // Verify field offsets
    assert(offsetof(tmc_callbacks_t, on_grid_update) == 8);
    assert(offsetof(tmc_callbacks_t, on_layout_change) == 16);
}
```

## Documentation Requirements

### Header Documentation
```c
/**
 * @brief Create a new tmux server instance
 * @param server Output pointer for server handle
 * @param config Configuration (caller owns memory)
 * @return TMC_OK on success, error code otherwise
 * 
 * @since 1.0
 * @thread-safe No
 * @lifetime Caller must call tmc_server_free()
 * 
 * @abi-stable YES
 * @abi-version 1.0+
 */
TMC_API tmc_err_t tmc_server_new(
    tmc_server_t** server,
    const tmc_config_t* config
);
```

### Change Log
Maintain `ABI_CHANGES.md`:
```markdown
# ABI Change Log

## Version 1.2 (2024-03-01)
### Added
- `tmc_command_async()` for non-blocking commands
- `on_frame` callback in tmc_callbacks_t

### Changed
- No breaking changes

### Size Changes
- tmc_callbacks_t: 48 -> 56 bytes (new optional callback)
```

## Summary

This ABI stability strategy ensures:
1. **Forward compatibility**: Older hosts work with newer libraries
2. **Backward compatibility**: Newer hosts work with older libraries (degraded)
3. **Runtime negotiation**: Features detected at runtime
4. **Clear evolution path**: Structured approach to adding features
5. **Platform independence**: Works across Linux/macOS/Windows
6. **Testability**: ABI changes are measurable and verifiable