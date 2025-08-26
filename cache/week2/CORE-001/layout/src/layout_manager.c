// layout_manager.c - Layout Management Implementation
// Purpose: Core layout algorithms and tree management
// Author: CORE-001 (c-tmux-specialist)
// Date: 2025-08-26
// Task: T-203 - Layout Management Callbacks
// Version: 1.0.0

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <math.h>
#include "../include/layout_callbacks.h"

// ============================================================================
// Thread Safety
// ============================================================================

typedef struct {
    pthread_mutex_t mutex;
    pthread_rwlock_t layout_lock;
} layout_locks_t;

// ============================================================================
// Helper Functions
// ============================================================================

// Get current time in microseconds
static uint64_t get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

// Find minimum size for a cell
static uint32_t get_min_size(layout_cell_t* cell, bool horizontal) {
    if (cell->type == LAYOUT_WINDOWPANE) {
        return horizontal ? 10 : 3;  // Minimum pane size
    }
    
    uint32_t min_size = 0;
    layout_cell_t* child = cell->children.first;
    
    while (child) {
        uint32_t child_min = get_min_size(child, horizontal);
        if (cell->type == LAYOUT_LEFTRIGHT && horizontal) {
            min_size += child_min;
        } else if (cell->type == LAYOUT_TOPBOTTOM && !horizontal) {
            min_size += child_min;
        } else {
            if (child_min > min_size) {
                min_size = child_min;
            }
        }
        child = child->next;
    }
    
    return min_size;
}

// ============================================================================
// Tmux Backend Implementation
// ============================================================================

static layout_cell_t* tmux_create_cell(layout_type_t type, layout_cell_t* parent) {
    layout_cell_t* cell = calloc(1, sizeof(layout_cell_t));
    if (!cell) return NULL;
    
    cell->type = type;
    cell->parent = parent;
    cell->sx = UINT32_MAX;
    cell->sy = UINT32_MAX;
    cell->xoff = UINT32_MAX;
    cell->yoff = UINT32_MAX;
    
    if (parent && parent->type != LAYOUT_WINDOWPANE) {
        // Add to parent's children
        if (!parent->children.first) {
            parent->children.first = cell;
            parent->children.last = cell;
        } else {
            parent->children.last->next = cell;
            cell->prev = parent->children.last;
            parent->children.last = cell;
        }
        parent->children.count++;
    }
    
    return cell;
}

static void tmux_free_cell(layout_cell_t* cell) {
    if (!cell) return;
    
    // Free children first
    if (cell->type != LAYOUT_WINDOWPANE) {
        layout_cell_t* child = cell->children.first;
        while (child) {
            layout_cell_t* next = child->next;
            tmux_free_cell(child);
            child = next;
        }
    }
    
    // Remove from parent's children
    if (cell->parent) {
        if (cell->prev) {
            cell->prev->next = cell->next;
        } else {
            cell->parent->children.first = cell->next;
        }
        
        if (cell->next) {
            cell->next->prev = cell->prev;
        } else {
            cell->parent->children.last = cell->prev;
        }
        
        cell->parent->children.count--;
    }
    
    free(cell);
}

static int tmux_split_pane(layout_cell_t* cell, split_direction_t dir, uint32_t size,
                           layout_cell_t** new_cell) {
    if (!cell || cell->type != LAYOUT_WINDOWPANE) {
        return -1;
    }
    
    uint64_t start_time = get_time_us();
    
    // Create container cell
    layout_type_t container_type = (dir == SPLIT_HORIZONTAL) ? 
                                   LAYOUT_LEFTRIGHT : LAYOUT_TOPBOTTOM;
    layout_cell_t* container = tmux_create_cell(container_type, cell->parent);
    if (!container) return -1;
    
    // Copy position from original cell
    container->xoff = cell->xoff;
    container->yoff = cell->yoff;
    container->sx = cell->sx;
    container->sy = cell->sy;
    
    // Replace cell with container in parent
    if (cell->parent) {
        if (cell->prev) {
            cell->prev->next = container;
        } else {
            cell->parent->children.first = container;
        }
        
        if (cell->next) {
            cell->next->prev = container;
        } else {
            cell->parent->children.last = container;
        }
        
        container->prev = cell->prev;
        container->next = cell->next;
    }
    
    // Reset cell links
    cell->parent = container;
    cell->prev = NULL;
    cell->next = NULL;
    
    // Create new pane
    layout_cell_t* new_pane = tmux_create_cell(LAYOUT_WINDOWPANE, container);
    if (!new_pane) {
        tmux_free_cell(container);
        return -1;
    }
    
    // Add cells to container
    container->children.first = cell;
    container->children.last = new_pane;
    container->children.count = 2;
    cell->next = new_pane;
    new_pane->prev = cell;
    
    // Calculate sizes
    if (dir == SPLIT_HORIZONTAL) {
        uint32_t total = container->sx;
        uint32_t first_size = (size > 0 && size < total) ? size : total / 2;
        
        cell->xoff = 0;
        cell->yoff = 0;
        cell->sx = first_size;
        cell->sy = container->sy;
        
        new_pane->xoff = first_size;
        new_pane->yoff = 0;
        new_pane->sx = total - first_size;
        new_pane->sy = container->sy;
    } else {
        uint32_t total = container->sy;
        uint32_t first_size = (size > 0 && size < total) ? size : total / 2;
        
        cell->xoff = 0;
        cell->yoff = 0;
        cell->sx = container->sx;
        cell->sy = first_size;
        
        new_pane->xoff = 0;
        new_pane->yoff = first_size;
        new_pane->sx = container->sx;
        new_pane->sy = total - first_size;
    }
    
    // Mark for redraw
    cell->needs_redraw = true;
    new_pane->needs_redraw = true;
    
    *new_cell = new_pane;
    
    uint64_t elapsed = get_time_us() - start_time;
    // Store timing for stats (simplified here)
    
    return 0;
}

static int tmux_close_pane(layout_cell_t* cell) {
    if (!cell || cell->type != LAYOUT_WINDOWPANE) {
        return -1;
    }
    
    layout_cell_t* parent = cell->parent;
    if (!parent) {
        // Can't close root pane
        return -1;
    }
    
    // If parent has only 2 children, collapse it
    if (parent->children.count == 2) {
        layout_cell_t* sibling = (cell->prev) ? cell->prev : cell->next;
        layout_cell_t* grandparent = parent->parent;
        
        if (grandparent) {
            // Replace parent with sibling in grandparent
            sibling->parent = grandparent;
            sibling->xoff = parent->xoff;
            sibling->yoff = parent->yoff;
            sibling->sx = parent->sx;
            sibling->sy = parent->sy;
            
            if (parent->prev) {
                parent->prev->next = sibling;
            } else {
                grandparent->children.first = sibling;
            }
            
            if (parent->next) {
                parent->next->prev = sibling;
            } else {
                grandparent->children.last = sibling;
            }
            
            sibling->prev = parent->prev;
            sibling->next = parent->next;
            
            // Remove cell from parent before freeing
            if (cell->prev) {
                cell->prev->next = cell->next;
            } else {
                parent->children.first = cell->next;
            }
            if (cell->next) {
                cell->next->prev = cell->prev;
            } else {
                parent->children.last = cell->prev;
            }
            parent->children.count--;
            
            free(cell);
            free(parent);
        }
    } else {
        // Just remove the cell and redistribute space
        uint32_t removed_size = (parent->type == LAYOUT_LEFTRIGHT) ? cell->sx : cell->sy;
        
        // Remove cell from parent
        if (cell->prev) {
            cell->prev->next = cell->next;
        } else {
            parent->children.first = cell->next;
        }
        
        if (cell->next) {
            cell->next->prev = cell->prev;
        } else {
            parent->children.last = cell->prev;
        }
        
        parent->children.count--;
        
        // Redistribute space
        uint32_t extra_per_child = removed_size / parent->children.count;
        layout_cell_t* child = parent->children.first;
        uint32_t offset = 0;
        
        while (child) {
            if (parent->type == LAYOUT_LEFTRIGHT) {
                child->xoff = offset;
                child->sx += extra_per_child;
                offset += child->sx;
            } else {
                child->yoff = offset;
                child->sy += extra_per_child;
                offset += child->sy;
            }
            child->needs_resize = true;
            child = child->next;
        }
        
        free(cell);
    }
    
    return 0;
}

static int tmux_resize_pane(layout_cell_t* cell, int32_t dx, int32_t dy) {
    if (!cell) return -1;
    
    uint64_t start_time = get_time_us();
    
    layout_cell_t* parent = cell->parent;
    if (!parent || parent->type == LAYOUT_WINDOWPANE) {
        return -1;
    }
    
    // Determine resize direction based on parent type
    if (parent->type == LAYOUT_LEFTRIGHT && dx != 0) {
        // Horizontal resize
        int32_t new_width = (int32_t)cell->sx + dx;
        uint32_t min_width = get_min_size(cell, true);
        
        if (new_width < min_width) {
            dx = min_width - cell->sx;
        }
        
        // Find adjacent cell
        layout_cell_t* adjacent = (dx > 0) ? cell->next : cell->prev;
        if (!adjacent) return -1;
        
        // Check adjacent cell minimum
        int32_t adj_new_width = (int32_t)adjacent->sx - dx;
        uint32_t adj_min_width = get_min_size(adjacent, true);
        
        if (adj_new_width < adj_min_width) {
            dx = adjacent->sx - adj_min_width;
        }
        
        // Apply resize
        cell->sx += dx;
        adjacent->sx -= dx;
        
        if (dx > 0 && cell->next) {
            cell->next->xoff += dx;
        } else if (dx < 0) {
            cell->xoff += dx;
        }
        
        cell->needs_resize = true;
        adjacent->needs_resize = true;
        
    } else if (parent->type == LAYOUT_TOPBOTTOM && dy != 0) {
        // Vertical resize
        int32_t new_height = (int32_t)cell->sy + dy;
        uint32_t min_height = get_min_size(cell, false);
        
        if (new_height < min_height) {
            dy = min_height - cell->sy;
        }
        
        // Find adjacent cell
        layout_cell_t* adjacent = (dy > 0) ? cell->next : cell->prev;
        if (!adjacent) return -1;
        
        // Check adjacent cell minimum
        int32_t adj_new_height = (int32_t)adjacent->sy - dy;
        uint32_t adj_min_height = get_min_size(adjacent, false);
        
        if (adj_new_height < adj_min_height) {
            dy = adjacent->sy - adj_min_height;
        }
        
        // Apply resize
        cell->sy += dy;
        adjacent->sy -= dy;
        
        if (dy > 0 && cell->next) {
            cell->next->yoff += dy;
        } else if (dy < 0) {
            cell->yoff += dy;
        }
        
        cell->needs_resize = true;
        adjacent->needs_resize = true;
    }
    
    uint64_t elapsed = get_time_us() - start_time;
    // Verify < 5ms requirement
    
    return 0;
}

static void tmux_resize_layout(layout_cell_t* root, uint32_t sx, uint32_t sy) {
    if (!root) return;
    
    uint64_t start_time = get_time_us();
    
    // Calculate scale factors
    double scale_x = (double)sx / root->sx;
    double scale_y = (double)sy / root->sy;
    
    // Recursive resize function
    void resize_cell(layout_cell_t* cell, uint32_t new_sx, uint32_t new_sy,
                    uint32_t new_xoff, uint32_t new_yoff) {
        cell->sx = new_sx;
        cell->sy = new_sy;
        cell->xoff = new_xoff;
        cell->yoff = new_yoff;
        cell->needs_resize = true;
        
        if (cell->type != LAYOUT_WINDOWPANE) {
            layout_cell_t* child = cell->children.first;
            uint32_t offset = 0;
            
            while (child) {
                if (cell->type == LAYOUT_LEFTRIGHT) {
                    uint32_t child_sx = (uint32_t)(child->sx * scale_x);
                    if (child->next == NULL) {
                        // Last child takes remaining space
                        child_sx = new_sx - offset;
                    }
                    resize_cell(child, child_sx, new_sy, offset, 0);
                    offset += child_sx;
                } else if (cell->type == LAYOUT_TOPBOTTOM) {
                    uint32_t child_sy = (uint32_t)(child->sy * scale_y);
                    if (child->next == NULL) {
                        // Last child takes remaining space
                        child_sy = new_sy - offset;
                    }
                    resize_cell(child, new_sx, child_sy, 0, offset);
                    offset += child_sy;
                }
                child = child->next;
            }
        }
    }
    
    resize_cell(root, sx, sy, 0, 0);
    
    uint64_t elapsed = get_time_us() - start_time;
    // Should be < 50ms for full layout change
}

// Tmux backend vtable
static layout_vtable_t tmux_layout_vtable = {
    .name = "tmux",
    .create_cell = tmux_create_cell,
    .free_cell = tmux_free_cell,
    .split_pane = tmux_split_pane,
    .close_pane = tmux_close_pane,
    .resize_pane = tmux_resize_pane,
    .resize_layout = tmux_resize_layout
    // Other functions to be implemented as needed
};

// ============================================================================
// Ghostty Backend Stub
// ============================================================================

static layout_vtable_t ghostty_layout_vtable = {
    .name = "ghostty",
    .create_cell = tmux_create_cell,  // Reuse for now
    .free_cell = tmux_free_cell,      // Reuse for now
    // Ghostty-specific optimizations to be added
};

// ============================================================================
// Layout Router Implementation
// ============================================================================

layout_router_t* global_layout_router = NULL;

layout_router_t* layout_router_init(layout_mode_t mode,
                                    event_loop_router_t* event_router,
                                    grid_router_t* grid_router) {
    layout_router_t* router = calloc(1, sizeof(layout_router_t));
    if (!router) return NULL;
    
    // Initialize locks
    layout_locks_t* locks = calloc(1, sizeof(layout_locks_t));
    pthread_mutex_init(&locks->mutex, NULL);
    pthread_rwlock_init(&locks->layout_lock, NULL);
    router->mutex = locks;
    
    // Set mode and vtable
    router->mode = mode;
    switch (mode) {
        case LAYOUT_MODE_TMUX:
            router->vtable = &tmux_layout_vtable;
            break;
        case LAYOUT_MODE_GHOSTTY:
            router->vtable = &ghostty_layout_vtable;
            break;
        case LAYOUT_MODE_HYBRID:
            router->vtable = &tmux_layout_vtable;  // Start with tmux
            break;
    }
    
    // Store event and grid routers
    router->event_router = event_router;
    router->grid_router = grid_router;
    
    // Set as global if not already set
    if (!global_layout_router) {
        global_layout_router = router;
    }
    
    return router;
}

void layout_router_cleanup(layout_router_t* router) {
    if (!router) return;
    
    layout_locks_t* locks = (layout_locks_t*)router->mutex;
    
    // Free layout tree
    if (router->root && router->vtable && router->vtable->free_cell) {
        router->vtable->free_cell(router->root);
    }
    
    // Clear global if this was it
    if (global_layout_router == router) {
        global_layout_router = NULL;
    }
    
    // Cleanup locks
    pthread_mutex_destroy(&locks->mutex);
    pthread_rwlock_destroy(&locks->layout_lock);
    free(locks);
    
    free(router);
}

layout_cell_t* layout_create_root(layout_router_t* router, uint32_t sx, uint32_t sy) {
    if (!router || !router->vtable || !router->vtable->create_cell) {
        return NULL;
    }
    
    layout_locks_t* locks = (layout_locks_t*)router->mutex;
    pthread_rwlock_wrlock(&locks->layout_lock);
    
    // Create root as a window pane
    layout_cell_t* root = router->vtable->create_cell(LAYOUT_WINDOWPANE, NULL);
    if (root) {
        root->xoff = 0;
        root->yoff = 0;
        root->sx = sx;
        root->sy = sy;
        router->root = root;
        router->sx = sx;
        router->sy = sy;
        router->pane_count = 1;
        router->generation++;
    }
    
    pthread_rwlock_unlock(&locks->layout_lock);
    
    return root;
}

layout_cell_t* layout_split_pane(layout_router_t* router, layout_cell_t* cell,
                                 split_direction_t dir, uint32_t size) {
    if (!router || !router->vtable || !router->vtable->split_pane) {
        return NULL;
    }
    
    layout_locks_t* locks = (layout_locks_t*)router->mutex;
    pthread_rwlock_wrlock(&locks->layout_lock);
    
    layout_cell_t* new_cell = NULL;
    uint64_t start_time = get_time_us();
    
    int result = router->vtable->split_pane(cell, dir, size, &new_cell);
    
    if (result == 0 && new_cell) {
        router->pane_count++;
        router->generation++;
        router->last_change_time_us = get_time_us() - start_time;
    }
    
    pthread_rwlock_unlock(&locks->layout_lock);
    
    return new_cell;
}

int layout_close_pane(layout_router_t* router, layout_cell_t* cell) {
    if (!router || !router->vtable || !router->vtable->close_pane) {
        return -1;
    }
    
    layout_locks_t* locks = (layout_locks_t*)router->mutex;
    pthread_rwlock_wrlock(&locks->layout_lock);
    
    uint64_t start_time = get_time_us();
    
    int result = router->vtable->close_pane(cell);
    
    if (result == 0) {
        router->pane_count--;
        router->generation++;
        router->last_change_time_us = get_time_us() - start_time;
    }
    
    pthread_rwlock_unlock(&locks->layout_lock);
    
    return result;
}

int layout_resize_pane(layout_router_t* router, layout_cell_t* cell,
                      int32_t dx, int32_t dy) {
    if (!router || !router->vtable || !router->vtable->resize_pane) {
        return -1;
    }
    
    layout_locks_t* locks = (layout_locks_t*)router->mutex;
    pthread_rwlock_wrlock(&locks->layout_lock);
    
    uint64_t start_time = get_time_us();
    
    int result = router->vtable->resize_pane(cell, dx, dy);
    
    if (result == 0) {
        router->generation++;
        router->last_change_time_us = get_time_us() - start_time;
    }
    
    pthread_rwlock_unlock(&locks->layout_lock);
    
    return result;
}

void layout_resize(layout_router_t* router, uint32_t sx, uint32_t sy) {
    if (!router || !router->vtable || !router->vtable->resize_layout) {
        return;
    }
    
    layout_locks_t* locks = (layout_locks_t*)router->mutex;
    pthread_rwlock_wrlock(&locks->layout_lock);
    
    uint64_t start_time = get_time_us();
    
    router->vtable->resize_layout(router->root, sx, sy);
    router->sx = sx;
    router->sy = sy;
    router->generation++;
    router->last_change_time_us = get_time_us() - start_time;
    
    pthread_rwlock_unlock(&locks->layout_lock);
}