// dirty_tracking.c - High-Performance Dirty Region Tracking
// Purpose: Minimize redraws by tracking changed regions efficiently
// Author: CORE-002 (libtmux-core-developer)
// Date: 2025-08-26
// Task: T-202 - Grid Operations Batch Optimization
// Version: 1.0.0

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>
#include "../include/grid_callbacks.h"

// Hierarchical dirty tracking constants
#define TILE_SIZE 16          // 16x16 cell tiles
#define MAX_DIRTY_RECTS 32    // Maximum individual dirty rectangles
#define COALESCE_THRESHOLD 4  // Min rects before coalescing

// Dirty tile bitmap for fast lookup
typedef struct {
    uint64_t* bitmap;         // Bit per tile
    uint32_t width_tiles;
    uint32_t height_tiles;
    uint32_t bitmap_size;
} dirty_tilemap_t;

// Individual dirty rectangle
typedef struct dirty_rect {
    uint32_t x, y;
    uint32_t width, height;
    uint64_t timestamp;       // For age-based coalescing
} dirty_rect_t;

// Advanced dirty tracking structure
typedef struct dirty_tracker {
    // Current aggregated dirty region
    dirty_region_t current;
    
    // Individual rectangles for fine-grained tracking
    dirty_rect_t rects[MAX_DIRTY_RECTS];
    size_t rect_count;
    
    // Hierarchical tile tracking
    dirty_tilemap_t tilemap;
    
    // Statistics
    struct {
        uint64_t total_marks;
        uint64_t coalesced;
        uint64_t full_redraws;
        uint64_t partial_redraws;
        uint64_t tiles_dirtied;
    } stats;
    
    // Configuration
    struct {
        bool enabled;
        bool use_tiles;
        bool auto_coalesce;
        uint32_t coalesce_threshold;
        double full_redraw_threshold;  // % of screen dirty before full redraw
    } config;
    
    // Thread safety
    pthread_spinlock_t lock;  // Spinlock for low-contention scenarios
    
    // Generation tracking for versioning
    uint64_t generation;
    uint64_t last_flush_generation;
} dirty_tracker_t;

// ============================================================================
// Tile Operations
// ============================================================================

static inline uint32_t xy_to_tile_index(dirty_tilemap_t* tilemap, uint32_t x, uint32_t y) {
    uint32_t tile_x = x / TILE_SIZE;
    uint32_t tile_y = y / TILE_SIZE;
    return tile_y * tilemap->width_tiles + tile_x;
}

static inline void set_tile_dirty(dirty_tilemap_t* tilemap, uint32_t tile_index) {
    uint32_t word = tile_index / 64;
    uint32_t bit = tile_index % 64;
    tilemap->bitmap[word] |= (1ULL << bit);
}

static inline bool is_tile_dirty(dirty_tilemap_t* tilemap, uint32_t tile_index) {
    uint32_t word = tile_index / 64;
    uint32_t bit = tile_index % 64;
    return (tilemap->bitmap[word] & (1ULL << bit)) != 0;
}

static void mark_tiles_dirty(dirty_tilemap_t* tilemap, 
                             uint32_t x, uint32_t y, 
                             uint32_t width, uint32_t height) {
    uint32_t start_tile_x = x / TILE_SIZE;
    uint32_t start_tile_y = y / TILE_SIZE;
    uint32_t end_tile_x = (x + width - 1) / TILE_SIZE;
    uint32_t end_tile_y = (y + height - 1) / TILE_SIZE;
    
    for (uint32_t ty = start_tile_y; ty <= end_tile_y && ty < tilemap->height_tiles; ty++) {
        for (uint32_t tx = start_tile_x; tx <= end_tile_x && tx < tilemap->width_tiles; tx++) {
            uint32_t index = ty * tilemap->width_tiles + tx;
            set_tile_dirty(tilemap, index);
        }
    }
}

static void clear_tilemap(dirty_tilemap_t* tilemap) {
    memset(tilemap->bitmap, 0, tilemap->bitmap_size * sizeof(uint64_t));
}

// Count dirty tiles using popcount
static uint32_t count_dirty_tiles(dirty_tilemap_t* tilemap) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < tilemap->bitmap_size; i++) {
        count += __builtin_popcountll(tilemap->bitmap[i]);
    }
    return count;
}

// ============================================================================
// Rectangle Operations
// ============================================================================

static inline bool rects_overlap(const dirty_rect_t* r1, const dirty_rect_t* r2) {
    return !(r1->x + r1->width <= r2->x || r2->x + r2->width <= r1->x ||
             r1->y + r1->height <= r2->y || r2->y + r2->height <= r1->y);
}

static inline bool rects_adjacent(const dirty_rect_t* r1, const dirty_rect_t* r2) {
    // Check if rectangles are adjacent (touching edges)
    bool x_adjacent = (r1->x + r1->width == r2->x) || (r2->x + r2->width == r1->x);
    bool y_adjacent = (r1->y + r1->height == r2->y) || (r2->y + r2->height == r1->y);
    
    // Rectangles must overlap in one dimension and be adjacent in the other
    bool x_overlap = !(r1->x + r1->width < r2->x || r2->x + r2->width < r1->x);
    bool y_overlap = !(r1->y + r1->height < r2->y || r2->y + r2->height < r1->y);
    
    return (x_adjacent && y_overlap) || (y_adjacent && x_overlap);
}

static dirty_rect_t merge_rects(const dirty_rect_t* r1, const dirty_rect_t* r2) {
    dirty_rect_t merged;
    merged.x = (r1->x < r2->x) ? r1->x : r2->x;
    merged.y = (r1->y < r2->y) ? r1->y : r2->y;
    
    uint32_t x_max = (r1->x + r1->width > r2->x + r2->width) ? 
                     r1->x + r1->width : r2->x + r2->width;
    uint32_t y_max = (r1->y + r1->height > r2->y + r2->height) ? 
                     r1->y + r1->height : r2->y + r2->height;
    
    merged.width = x_max - merged.x;
    merged.height = y_max - merged.y;
    merged.timestamp = (r1->timestamp > r2->timestamp) ? r1->timestamp : r2->timestamp;
    
    return merged;
}

// Coalesce overlapping and adjacent rectangles
static void coalesce_rects(dirty_tracker_t* tracker) {
    if (tracker->rect_count < tracker->config.coalesce_threshold) {
        return;
    }
    
    bool changed;
    do {
        changed = false;
        for (size_t i = 0; i < tracker->rect_count && !changed; i++) {
            for (size_t j = i + 1; j < tracker->rect_count; j++) {
                if (rects_overlap(&tracker->rects[i], &tracker->rects[j]) ||
                    rects_adjacent(&tracker->rects[i], &tracker->rects[j])) {
                    
                    // Merge rectangles
                    tracker->rects[i] = merge_rects(&tracker->rects[i], &tracker->rects[j]);
                    
                    // Remove the merged rectangle
                    if (j < tracker->rect_count - 1) {
                        memmove(&tracker->rects[j], &tracker->rects[j + 1],
                               (tracker->rect_count - j - 1) * sizeof(dirty_rect_t));
                    }
                    tracker->rect_count--;
                    tracker->stats.coalesced++;
                    changed = true;
                    break;
                }
            }
        }
    } while (changed && tracker->rect_count > 1);
}

// ============================================================================
// Public API Implementation
// ============================================================================

dirty_tracker_t* dirty_tracker_create(uint32_t width, uint32_t height) {
    dirty_tracker_t* tracker = calloc(1, sizeof(dirty_tracker_t));
    if (!tracker) return NULL;
    
    // Initialize configuration
    tracker->config.enabled = true;
    tracker->config.use_tiles = true;
    tracker->config.auto_coalesce = true;
    tracker->config.coalesce_threshold = COALESCE_THRESHOLD;
    tracker->config.full_redraw_threshold = 0.5;  // 50% of screen
    
    // Initialize tilemap
    tracker->tilemap.width_tiles = (width + TILE_SIZE - 1) / TILE_SIZE;
    tracker->tilemap.height_tiles = (height + TILE_SIZE - 1) / TILE_SIZE;
    tracker->tilemap.bitmap_size = 
        ((tracker->tilemap.width_tiles * tracker->tilemap.height_tiles) + 63) / 64;
    tracker->tilemap.bitmap = calloc(tracker->tilemap.bitmap_size, sizeof(uint64_t));
    
    if (!tracker->tilemap.bitmap) {
        free(tracker);
        return NULL;
    }
    
    // Initialize spinlock
    pthread_spin_init(&tracker->lock, PTHREAD_PROCESS_PRIVATE);
    
    return tracker;
}

void dirty_tracker_destroy(dirty_tracker_t* tracker) {
    if (!tracker) return;
    
    free(tracker->tilemap.bitmap);
    pthread_spin_destroy(&tracker->lock);
    free(tracker);
}

void dirty_tracker_mark(dirty_tracker_t* tracker,
                        uint32_t x, uint32_t y,
                        uint32_t width, uint32_t height) {
    if (!tracker || !tracker->config.enabled) return;
    
    pthread_spin_lock(&tracker->lock);
    
    tracker->stats.total_marks++;
    tracker->generation++;
    
    // Update aggregate region
    uint32_t x_max = x + width - 1;
    uint32_t y_max = y + height - 1;
    
    if (tracker->current.generation == 0) {
        // First dirty region
        tracker->current.x_min = x;
        tracker->current.y_min = y;
        tracker->current.x_max = x_max;
        tracker->current.y_max = y_max;
    } else {
        // Expand existing region
        if (x < tracker->current.x_min) tracker->current.x_min = x;
        if (y < tracker->current.y_min) tracker->current.y_min = y;
        if (x_max > tracker->current.x_max) tracker->current.x_max = x_max;
        if (y_max > tracker->current.y_max) tracker->current.y_max = y_max;
    }
    tracker->current.generation = tracker->generation;
    
    // Mark tiles if enabled
    if (tracker->config.use_tiles) {
        mark_tiles_dirty(&tracker->tilemap, x, y, width, height);
    }
    
    // Add to rectangle list
    if (tracker->rect_count < MAX_DIRTY_RECTS) {
        dirty_rect_t* rect = &tracker->rects[tracker->rect_count++];
        rect->x = x;
        rect->y = y;
        rect->width = width;
        rect->height = height;
        rect->timestamp = tracker->generation;
        
        // Auto-coalesce if threshold reached
        if (tracker->config.auto_coalesce && 
            tracker->rect_count >= tracker->config.coalesce_threshold) {
            coalesce_rects(tracker);
        }
    } else {
        // Too many rectangles - trigger full redraw
        tracker->current.full_redraw = true;
        tracker->stats.full_redraws++;
    }
    
    // Check if we should do full redraw based on coverage
    if (!tracker->current.full_redraw && tracker->config.use_tiles) {
        uint32_t total_tiles = tracker->tilemap.width_tiles * tracker->tilemap.height_tiles;
        uint32_t dirty_tiles = count_dirty_tiles(&tracker->tilemap);
        
        if ((double)dirty_tiles / total_tiles > tracker->config.full_redraw_threshold) {
            tracker->current.full_redraw = true;
            tracker->stats.full_redraws++;
        }
    }
    
    pthread_spin_unlock(&tracker->lock);
}

bool dirty_tracker_get_region(dirty_tracker_t* tracker, dirty_region_t* region) {
    if (!tracker || !region) return false;
    
    pthread_spin_lock(&tracker->lock);
    
    if (tracker->current.generation == tracker->last_flush_generation) {
        // No changes since last flush
        pthread_spin_unlock(&tracker->lock);
        return false;
    }
    
    *region = tracker->current;
    
    // Provide additional rectangle information if available
    if (!region->full_redraw && tracker->rect_count > 0) {
        tracker->stats.partial_redraws++;
    }
    
    pthread_spin_unlock(&tracker->lock);
    return true;
}

void dirty_tracker_clear(dirty_tracker_t* tracker) {
    if (!tracker) return;
    
    pthread_spin_lock(&tracker->lock);
    
    // Clear current region
    memset(&tracker->current, 0, sizeof(dirty_region_t));
    
    // Clear rectangles
    tracker->rect_count = 0;
    
    // Clear tilemap
    if (tracker->config.use_tiles) {
        clear_tilemap(&tracker->tilemap);
    }
    
    tracker->last_flush_generation = tracker->generation;
    
    pthread_spin_unlock(&tracker->lock);
}

// Get individual dirty rectangles for fine-grained updates
size_t dirty_tracker_get_rects(dirty_tracker_t* tracker,
                               dirty_rect_t* rects,
                               size_t max_rects) {
    if (!tracker || !rects || max_rects == 0) return 0;
    
    pthread_spin_lock(&tracker->lock);
    
    size_t count = (tracker->rect_count < max_rects) ? 
                   tracker->rect_count : max_rects;
    
    if (count > 0) {
        memcpy(rects, tracker->rects, count * sizeof(dirty_rect_t));
    }
    
    pthread_spin_unlock(&tracker->lock);
    return count;
}

// Get dirty tiles for tile-based rendering
size_t dirty_tracker_get_dirty_tiles(dirty_tracker_t* tracker,
                                     uint32_t* tile_indices,
                                     size_t max_tiles) {
    if (!tracker || !tile_indices || max_tiles == 0 || !tracker->config.use_tiles) {
        return 0;
    }
    
    pthread_spin_lock(&tracker->lock);
    
    size_t count = 0;
    uint32_t total_tiles = tracker->tilemap.width_tiles * tracker->tilemap.height_tiles;
    
    for (uint32_t i = 0; i < total_tiles && count < max_tiles; i++) {
        if (is_tile_dirty(&tracker->tilemap, i)) {
            tile_indices[count++] = i;
        }
    }
    
    tracker->stats.tiles_dirtied = count;
    
    pthread_spin_unlock(&tracker->lock);
    return count;
}

// Configuration functions
void dirty_tracker_enable(dirty_tracker_t* tracker, bool enable) {
    if (!tracker) return;
    tracker->config.enabled = enable;
}

void dirty_tracker_set_tile_mode(dirty_tracker_t* tracker, bool use_tiles) {
    if (!tracker) return;
    tracker->config.use_tiles = use_tiles;
}

void dirty_tracker_set_auto_coalesce(dirty_tracker_t* tracker, bool enable) {
    if (!tracker) return;
    tracker->config.auto_coalesce = enable;
}

void dirty_tracker_set_full_redraw_threshold(dirty_tracker_t* tracker, double threshold) {
    if (!tracker) return;
    if (threshold >= 0.0 && threshold <= 1.0) {
        tracker->config.full_redraw_threshold = threshold;
    }
}

// Statistics
void dirty_tracker_get_stats(dirty_tracker_t* tracker,
                             uint64_t* total_marks,
                             uint64_t* coalesced,
                             uint64_t* full_redraws,
                             uint64_t* partial_redraws) {
    if (!tracker) return;
    
    pthread_spin_lock(&tracker->lock);
    
    if (total_marks) *total_marks = tracker->stats.total_marks;
    if (coalesced) *coalesced = tracker->stats.coalesced;
    if (full_redraws) *full_redraws = tracker->stats.full_redraws;
    if (partial_redraws) *partial_redraws = tracker->stats.partial_redraws;
    
    pthread_spin_unlock(&tracker->lock);
}

void dirty_tracker_reset_stats(dirty_tracker_t* tracker) {
    if (!tracker) return;
    
    pthread_spin_lock(&tracker->lock);
    memset(&tracker->stats, 0, sizeof(tracker->stats));
    pthread_spin_unlock(&tracker->lock);
}

// ============================================================================
// Optimization Helpers
// ============================================================================

// Check if a region is entirely within the dirty area
bool dirty_tracker_contains_region(dirty_tracker_t* tracker,
                                   uint32_t x, uint32_t y,
                                   uint32_t width, uint32_t height) {
    if (!tracker || !tracker->config.enabled) return false;
    
    pthread_spin_lock(&tracker->lock);
    
    bool contained = false;
    if (tracker->current.full_redraw) {
        contained = true;
    } else if (tracker->current.generation > 0) {
        contained = (x >= tracker->current.x_min &&
                    y >= tracker->current.y_min &&
                    x + width - 1 <= tracker->current.x_max &&
                    y + height - 1 <= tracker->current.y_max);
    }
    
    pthread_spin_unlock(&tracker->lock);
    return contained;
}

// Check if a region intersects with dirty area
bool dirty_tracker_intersects_region(dirty_tracker_t* tracker,
                                     uint32_t x, uint32_t y,
                                     uint32_t width, uint32_t height) {
    if (!tracker || !tracker->config.enabled) return false;
    
    pthread_spin_lock(&tracker->lock);
    
    bool intersects = false;
    if (tracker->current.full_redraw) {
        intersects = true;
    } else if (tracker->current.generation > 0) {
        uint32_t x_max = x + width - 1;
        uint32_t y_max = y + height - 1;
        
        intersects = !(x > tracker->current.x_max ||
                      x_max < tracker->current.x_min ||
                      y > tracker->current.y_max ||
                      y_max < tracker->current.y_min);
    }
    
    pthread_spin_unlock(&tracker->lock);
    return intersects;
}

// Force a full redraw
void dirty_tracker_force_full_redraw(dirty_tracker_t* tracker) {
    if (!tracker) return;
    
    pthread_spin_lock(&tracker->lock);
    
    tracker->current.full_redraw = true;
    tracker->current.x_min = 0;
    tracker->current.y_min = 0;
    tracker->current.x_max = UINT32_MAX;
    tracker->current.y_max = UINT32_MAX;
    tracker->current.generation = ++tracker->generation;
    tracker->stats.full_redraws++;
    
    pthread_spin_unlock(&tracker->lock);
}