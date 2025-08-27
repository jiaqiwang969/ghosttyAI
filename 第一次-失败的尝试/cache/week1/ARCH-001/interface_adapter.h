// interface_adapter.h - 接口适配器头文件
// Author: ARCH-001 (System Architect)
// Purpose: 接口适配器声明
// Version: 2.0.0

#ifndef INTERFACE_ADAPTER_H
#define INTERFACE_ADAPTER_H

#include "interface_compatibility.h"

// ============================================================================
// Standard Interface Functions (Canonical)
// ============================================================================

// TTY Hooks - Standard Interface
// Already declared in interface_compatibility.h
void tty_hooks_cleanup(void);
int tty_hooks_install(const char* name, void* hook_fn);

// Backend Router - Standard Interface
int backend_router_create(struct backend_router** router);
void backend_router_destroy(struct backend_router* router);
// backend_router_register already declared in interface_compatibility.h

// Interface Registry
int interface_registry_init(void);
void interface_registry_cleanup(void);

// Version Management
interface_version_t* interface_get_version(void);
int interface_is_version_supported(uint32_t major, uint32_t minor);

// Self-Test
int interface_self_test(void);

// ============================================================================
// Deprecated Functions (Compatibility Layer)
// ============================================================================

// These are declared in interface_compatibility.h and implemented in interface_adapter.c

#endif /* INTERFACE_ADAPTER_H */