// backend_ghostty.h - Public Interface for Ghostty Backend
// Purpose: Header file for Ghostty UI backend integration
// Author: INTG-001 (Zig-Ghostty Integration Specialist)
// Date: 2025-08-25
// Version: 1.0.0

#ifndef BACKEND_GHOSTTY_H
#define BACKEND_GHOSTTY_H

#include <stdint.h>
#include <stdbool.h>
#include "../ARCH-001/ui_backend.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Ghostty Backend Creation
// ============================================================================

/**
 * Create a new Ghostty backend instance.
 * 
 * @param requested_caps Requested capabilities (can be NULL for defaults)
 * @return Pointer to the new backend, or NULL on failure
 */
struct ui_backend* ghostty_backend_create(const ui_capabilities_t* requested_caps);

/**
 * Destroy a Ghostty backend instance.
 * 
 * @param backend The backend to destroy
 */
void ghostty_backend_destroy(struct ui_backend* backend);

// ============================================================================
// Configuration Functions
// ============================================================================

/**
 * Set immediate mode (bypass frame batching).
 * 
 * @param backend The backend instance
 * @param immediate True to enable immediate mode, false for batching
 */
void ghostty_backend_set_immediate_mode(struct ui_backend* backend, bool immediate);

/**
 * Enable or disable grid optimization.
 * 
 * @param backend The backend instance
 * @param enabled True to enable grid optimization
 */
void ghostty_backend_set_grid_optimization(struct ui_backend* backend, bool enabled);

/**
 * Set the maximum batch size for frame aggregation.
 * 
 * @param backend The backend instance
 * @param max_batch Maximum number of spans per batch
 */
void ghostty_backend_set_max_batch_size(struct ui_backend* backend, uint32_t max_batch);

// ============================================================================
// Statistics and Monitoring
// ============================================================================

/**
 * Get backend statistics.
 * 
 * @param backend The backend instance
 * @param frames_sent Output: Number of frames sent (can be NULL)
 * @param cells_updated Output: Number of cells updated (can be NULL)
 * @param frames_batched Output: Number of frames batched (can be NULL)
 */
void ghostty_backend_get_statistics(struct ui_backend* backend,
                                    uint64_t* frames_sent,
                                    uint64_t* cells_updated,
                                    uint64_t* frames_batched);

/**
 * Reset backend statistics.
 * 
 * @param backend The backend instance
 */
void ghostty_backend_reset_statistics(struct ui_backend* backend);

// ============================================================================
// Advanced Features
// ============================================================================

/**
 * Force a frame flush.
 * 
 * @param backend The backend instance
 */
void ghostty_backend_flush_frame(struct ui_backend* backend);

/**
 * Get the dirty region bitmap.
 * 
 * @param backend The backend instance
 * @param rows Output: Bitmap of dirty rows (must be pre-allocated)
 * @param cols Output: Bitmap of dirty columns (must be pre-allocated)
 * @param rows_size Size of rows bitmap in bytes
 * @param cols_size Size of cols bitmap in bytes
 */
void ghostty_backend_get_dirty_region(struct ui_backend* backend,
                                      uint32_t* rows,
                                      uint32_t* cols,
                                      size_t rows_size,
                                      size_t cols_size);

/**
 * Clear the dirty region tracking.
 * 
 * @param backend The backend instance
 */
void ghostty_backend_clear_dirty_region(struct ui_backend* backend);

// ============================================================================
// Integration Helpers
// ============================================================================

/**
 * Register the Ghostty backend with the global router.
 * 
 * @return 0 on success, negative error code on failure
 */
int ghostty_backend_register_global(void);

/**
 * Unregister the Ghostty backend from the global router.
 */
void ghostty_backend_unregister_global(void);

/**
 * Check if Ghostty backend is available.
 * 
 * @return true if available, false otherwise
 */
bool ghostty_backend_is_available(void);

/**
 * Get the version string of the Ghostty backend.
 * 
 * @return Version string (do not free)
 */
const char* ghostty_backend_get_version(void);

#ifdef __cplusplus
}
#endif

#endif // BACKEND_GHOSTTY_H