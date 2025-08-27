// pane_operations.c - Pane Operations Implementation
// Purpose: Pane management, focus, zoom, and synchronization
// Author: CORE-001 (c-tmux-specialist)
// Date: 2025-08-26
// Task: T-203 - Layout Management Callbacks
// Version: 1.0.0

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include "../include/layout_callbacks.h"

// ============================================================================
// Layout Preset Implementations
// ============================================================================

typedef struct {
    const char* name;
    layout_preset_t preset;
    void (*apply)(layout_router_t* router);
} layout_preset_def_t;

// Even horizontal layout - all panes same width
static void apply_even_horizontal(layout_router_t* router) {
    if (!router || !router->root) return;
    
    layout_cell_t* root = router->root;
    
    // If only one pane, nothing to do
    if (router->pane_count <= 1) return;
    
    // Convert root to container if needed
    if (root->type == LAYOUT_WINDOWPANE) {
        root->type = LAYOUT_LEFTRIGHT;
    }
    
    // Calculate equal width for each pane
    uint32_t pane_width = router->sx / router->pane_count;
    uint32_t remainder = router->sx % router->pane_count;
    
    // Arrange panes horizontally
    layout_cell_t* child = root->children.first;
    uint32_t xoff = 0;
    uint32_t index = 0;
    
    while (child) {
        child->xoff = xoff;
        child->yoff = 0;
        child->sx = pane_width + (index < remainder ? 1 : 0);
        child->sy = router->sy;
        child->needs_resize = true;
        
        xoff += child->sx;
        index++;
        child = child->next;
    }
}

// Even vertical layout - all panes same height
static void apply_even_vertical(layout_router_t* router) {
    if (!router || !router->root) return;
    
    layout_cell_t* root = router->root;
    
    // If only one pane, nothing to do
    if (router->pane_count <= 1) return;
    
    // Convert root to container if needed
    if (root->type == LAYOUT_WINDOWPANE) {
        root->type = LAYOUT_TOPBOTTOM;
    }
    
    // Calculate equal height for each pane
    uint32_t pane_height = router->sy / router->pane_count;
    uint32_t remainder = router->sy % router->pane_count;
    
    // Arrange panes vertically
    layout_cell_t* child = root->children.first;
    uint32_t yoff = 0;
    uint32_t index = 0;
    
    while (child) {
        child->xoff = 0;
        child->yoff = yoff;
        child->sx = router->sx;
        child->sy = pane_height + (index < remainder ? 1 : 0);
        child->needs_resize = true;
        
        yoff += child->sy;
        index++;
        child = child->next;
    }
}

// Main horizontal layout - main pane on top, others below
static void apply_main_horizontal(layout_router_t* router) {
    if (!router || !router->root || router->pane_count <= 1) return;
    
    layout_cell_t* root = router->root;
    
    // Main pane takes 60% of height
    uint32_t main_height = (router->sy * 60) / 100;
    uint32_t other_height = router->sy - main_height;
    
    // Convert root to vertical container
    if (root->type != LAYOUT_TOPBOTTOM) {
        root->type = LAYOUT_TOPBOTTOM;
    }
    
    // First child is main pane
    layout_cell_t* main_pane = root->children.first;
    main_pane->xoff = 0;
    main_pane->yoff = 0;
    main_pane->sx = router->sx;
    main_pane->sy = main_height;
    main_pane->needs_resize = true;
    
    // Create horizontal container for other panes if needed
    if (router->pane_count > 2) {
        layout_cell_t* container = main_pane->next;
        if (container->type != LAYOUT_LEFTRIGHT) {
            container->type = LAYOUT_LEFTRIGHT;
        }
        
        container->xoff = 0;
        container->yoff = main_height;
        container->sx = router->sx;
        container->sy = other_height;
        
        // Distribute other panes horizontally
        uint32_t other_count = router->pane_count - 1;
        uint32_t pane_width = router->sx / other_count;
        
        layout_cell_t* child = container->children.first;
        uint32_t xoff = 0;
        
        while (child) {
            child->xoff = xoff;
            child->yoff = 0;
            child->sx = pane_width;
            child->sy = other_height;
            child->needs_resize = true;
            
            xoff += pane_width;
            child = child->next;
        }
    } else {
        // Just one other pane
        layout_cell_t* other = main_pane->next;
        other->xoff = 0;
        other->yoff = main_height;
        other->sx = router->sx;
        other->sy = other_height;
        other->needs_resize = true;
    }
}

// Main vertical layout - main pane on left, others on right
static void apply_main_vertical(layout_router_t* router) {
    if (!router || !router->root || router->pane_count <= 1) return;
    
    layout_cell_t* root = router->root;
    
    // Main pane takes 60% of width
    uint32_t main_width = (router->sx * 60) / 100;
    uint32_t other_width = router->sx - main_width;
    
    // Convert root to horizontal container
    if (root->type != LAYOUT_LEFTRIGHT) {
        root->type = LAYOUT_LEFTRIGHT;
    }
    
    // First child is main pane
    layout_cell_t* main_pane = root->children.first;
    main_pane->xoff = 0;
    main_pane->yoff = 0;
    main_pane->sx = main_width;
    main_pane->sy = router->sy;
    main_pane->needs_resize = true;
    
    // Create vertical container for other panes if needed
    if (router->pane_count > 2) {
        layout_cell_t* container = main_pane->next;
        if (container->type != LAYOUT_TOPBOTTOM) {
            container->type = LAYOUT_TOPBOTTOM;
        }
        
        container->xoff = main_width;
        container->yoff = 0;
        container->sx = other_width;
        container->sy = router->sy;
        
        // Distribute other panes vertically
        uint32_t other_count = router->pane_count - 1;
        uint32_t pane_height = router->sy / other_count;
        
        layout_cell_t* child = container->children.first;
        uint32_t yoff = 0;
        
        while (child) {
            child->xoff = 0;
            child->yoff = yoff;
            child->sx = other_width;
            child->sy = pane_height;
            child->needs_resize = true;
            
            yoff += pane_height;
            child = child->next;
        }
    } else {
        // Just one other pane
        layout_cell_t* other = main_pane->next;
        other->xoff = main_width;
        other->yoff = 0;
        other->sx = other_width;
        other->sy = router->sy;
        other->needs_resize = true;
    }
}

// Tiled layout - arrange panes in a grid
static void apply_tiled(layout_router_t* router) {
    if (!router || !router->root || router->pane_count <= 1) return;
    
    // Calculate grid dimensions
    uint32_t cols = 1;
    uint32_t rows = router->pane_count;
    
    // Try to make it square-ish
    while (cols * cols < router->pane_count) {
        cols++;
    }
    rows = (router->pane_count + cols - 1) / cols;
    
    uint32_t cell_width = router->sx / cols;
    uint32_t cell_height = router->sy / rows;
    
    // Arrange panes in grid
    layout_cell_t* root = router->root;
    layout_cell_t* pane = root;
    uint32_t index = 0;
    
    for (uint32_t row = 0; row < rows && pane; row++) {
        for (uint32_t col = 0; col < cols && pane; col++) {
            pane->xoff = col * cell_width;
            pane->yoff = row * cell_height;
            pane->sx = cell_width;
            pane->sy = cell_height;
            pane->needs_resize = true;
            
            index++;
            if (index < router->pane_count) {
                pane = pane->next;
            } else {
                break;
            }
        }
    }
}

static layout_preset_def_t layout_presets[] = {
    { "even-horizontal", LAYOUT_PRESET_EVEN_HORIZONTAL, apply_even_horizontal },
    { "even-vertical", LAYOUT_PRESET_EVEN_VERTICAL, apply_even_vertical },
    { "main-horizontal", LAYOUT_PRESET_MAIN_HORIZONTAL, apply_main_horizontal },
    { "main-vertical", LAYOUT_PRESET_MAIN_VERTICAL, apply_main_vertical },
    { "tiled", LAYOUT_PRESET_TILED, apply_tiled }
};

// ============================================================================
// Custom Layout String Parsing
// ============================================================================

// Calculate checksum of layout string
static uint16_t calculate_checksum(const char* layout) {
    uint16_t csum = 0;
    for (; *layout != '\0'; layout++) {
        csum = (csum >> 1) + ((csum & 1) << 15);
        csum += *layout;
    }
    return csum;
}

// Parse a single number from string
static int parse_number(const char** str) {
    int num = 0;
    while (isdigit(**str)) {
        num = num * 10 + (**str - '0');
        (*str)++;
    }
    return num;
}

// Parse layout cell from string
static layout_cell_t* parse_layout_cell(const char** str, layout_cell_t* parent) {
    // Format: WxH,X,Y[,ID]
    // Or for containers: WxH,X,Y{cells} or WxH,X,Y[cells]
    
    if (!str || !*str) return NULL;
    
    // Parse dimensions
    uint32_t sx = parse_number(str);
    if (**str != 'x') return NULL;
    (*str)++;
    
    uint32_t sy = parse_number(str);
    if (**str != ',') return NULL;
    (*str)++;
    
    // Parse position
    uint32_t xoff = parse_number(str);
    if (**str != ',') return NULL;
    (*str)++;
    
    uint32_t yoff = parse_number(str);
    
    // Determine cell type
    layout_type_t type;
    char bracket;
    
    if (**str == ',') {
        // Window pane with ID
        (*str)++;
        type = LAYOUT_WINDOWPANE;
    } else if (**str == '{' || **str == '}') {
        // Horizontal container
        bracket = **str;
        (*str)++;
        type = LAYOUT_LEFTRIGHT;
    } else if (**str == '[' || **str == ']') {
        // Vertical container
        bracket = **str;
        (*str)++;
        type = LAYOUT_TOPBOTTOM;
    } else {
        return NULL;
    }
    
    // Create cell
    layout_cell_t* cell = calloc(1, sizeof(layout_cell_t));
    if (!cell) return NULL;
    
    cell->type = type;
    cell->parent = parent;
    cell->sx = sx;
    cell->sy = sy;
    cell->xoff = xoff;
    cell->yoff = yoff;
    
    if (type == LAYOUT_WINDOWPANE) {
        // Parse pane ID
        cell->pane.id = parse_number(str);
    } else {
        // Parse children
        while (**str && **str != '}' && **str != ']') {
            layout_cell_t* child = parse_layout_cell(str, cell);
            if (!child) {
                free(cell);
                return NULL;
            }
            
            // Add child to list
            if (!cell->children.first) {
                cell->children.first = child;
                cell->children.last = child;
            } else {
                cell->children.last->next = child;
                child->prev = cell->children.last;
                cell->children.last = child;
            }
            cell->children.count++;
            
            if (**str == ',') {
                (*str)++;
            }
        }
        
        // Skip closing bracket
        if (**str == '}' || **str == ']') {
            (*str)++;
        }
    }
    
    return cell;
}

// ============================================================================
// Zoom Operations
// ============================================================================

static int zoom_pane_impl(layout_router_t* router, layout_cell_t* cell) {
    if (!router || !cell) return -1;
    
    // Can only zoom window panes
    if (cell->type != LAYOUT_WINDOWPANE) return -1;
    
    // Save current layout
    router->saved_layout = router->root;
    
    // Create new root with just the zoomed pane
    layout_cell_t* zoomed = calloc(1, sizeof(layout_cell_t));
    if (!zoomed) return -1;
    
    *zoomed = *cell;  // Copy pane info
    zoomed->parent = NULL;
    zoomed->prev = NULL;
    zoomed->next = NULL;
    zoomed->xoff = 0;
    zoomed->yoff = 0;
    zoomed->sx = router->sx;
    zoomed->sy = router->sy;
    zoomed->pane.zoomed = true;
    
    router->root = zoomed;
    router->zoomed_pane = zoomed;
    
    return 0;
}

static int unzoom_pane_impl(layout_router_t* router) {
    if (!router || !router->zoomed_pane) return -1;
    
    // Restore saved layout
    if (router->saved_layout) {
        router->root = router->saved_layout;
        router->saved_layout = NULL;
    }
    
    // Clear zoom state
    if (router->zoomed_pane) {
        router->zoomed_pane->pane.zoomed = false;
        free(router->zoomed_pane);
        router->zoomed_pane = NULL;
    }
    
    return 0;
}

// ============================================================================
// Navigation Functions
// ============================================================================

static layout_cell_t* find_pane_recursive(layout_cell_t* cell, uint32_t id) {
    if (!cell) return NULL;
    
    if (cell->type == LAYOUT_WINDOWPANE && cell->pane.id == id) {
        return cell;
    }
    
    if (cell->type != LAYOUT_WINDOWPANE) {
        layout_cell_t* child = cell->children.first;
        while (child) {
            layout_cell_t* found = find_pane_recursive(child, id);
            if (found) return found;
            child = child->next;
        }
    }
    
    return NULL;
}

static layout_cell_t* find_adjacent_impl(layout_cell_t* cell, int direction) {
    if (!cell || !cell->parent) return NULL;
    
    layout_cell_t* parent = cell->parent;
    
    // Direction: 0=up, 1=right, 2=down, 3=left
    switch (direction) {
        case 0: // Up
            if (parent->type == LAYOUT_TOPBOTTOM && cell->prev) {
                return cell->prev;
            }
            break;
            
        case 1: // Right
            if (parent->type == LAYOUT_LEFTRIGHT && cell->next) {
                return cell->next;
            }
            break;
            
        case 2: // Down
            if (parent->type == LAYOUT_TOPBOTTOM && cell->next) {
                return cell->next;
            }
            break;
            
        case 3: // Left
            if (parent->type == LAYOUT_LEFTRIGHT && cell->prev) {
                return cell->prev;
            }
            break;
    }
    
    // Try parent's siblings
    return find_adjacent_impl(parent, direction);
}

static layout_cell_t* find_by_position_recursive(layout_cell_t* cell, 
                                                 uint32_t x, uint32_t y) {
    if (!cell) return NULL;
    
    // Check if position is within this cell
    if (x >= cell->xoff && x < cell->xoff + cell->sx &&
        y >= cell->yoff && y < cell->yoff + cell->sy) {
        
        if (cell->type == LAYOUT_WINDOWPANE) {
            return cell;
        }
        
        // Check children
        layout_cell_t* child = cell->children.first;
        while (child) {
            layout_cell_t* found = find_by_position_recursive(child, x, y);
            if (found) return found;
            child = child->next;
        }
    }
    
    return NULL;
}

// ============================================================================
// Public API Extensions
// ============================================================================

int layout_apply_preset(layout_router_t* router, layout_preset_t preset) {
    if (!router || preset >= sizeof(layout_presets)/sizeof(layout_presets[0])) {
        return -1;
    }
    
    layout_preset_def_t* def = &layout_presets[preset];
    if (def->apply) {
        def->apply(router);
        router->generation++;
        return 0;
    }
    
    return -1;
}

int layout_parse_custom(layout_router_t* router, const char* layout_string) {
    if (!router || !layout_string) return -1;
    
    // Format: CHECKSUM,layout
    // Skip checksum
    const char* comma = strchr(layout_string, ',');
    if (!comma) return -1;
    
    const char* layout = comma + 1;
    layout_cell_t* new_root = parse_layout_cell(&layout, NULL);
    
    if (!new_root) return -1;
    
    // Replace current layout
    if (router->root && router->vtable && router->vtable->free_cell) {
        router->vtable->free_cell(router->root);
    }
    
    router->root = new_root;
    router->generation++;
    
    return 0;
}

char* layout_dump(layout_router_t* router) {
    if (!router || !router->root) return NULL;
    
    char buffer[8192];
    int pos = 0;
    
    // Recursive dump function
    void dump_cell(layout_cell_t* cell, char* buf, int* p) {
        if (!cell) return;
        
        // Write dimensions and position
        pos += snprintf(buf + *p, sizeof(buffer) - *p, "%ux%u,%u,%u",
                       cell->sx, cell->sy, cell->xoff, cell->yoff);
        
        if (cell->type == LAYOUT_WINDOWPANE) {
            // Write pane ID
            pos += snprintf(buf + *p, sizeof(buffer) - *p, ",%u", cell->pane.id);
        } else {
            // Write container bracket
            char open_bracket = (cell->type == LAYOUT_LEFTRIGHT) ? '{' : '[';
            char close_bracket = (cell->type == LAYOUT_LEFTRIGHT) ? '}' : ']';
            
            buf[(*p)++] = open_bracket;
            
            // Dump children
            layout_cell_t* child = cell->children.first;
            while (child) {
                dump_cell(child, buf, p);
                if (child->next) {
                    buf[(*p)++] = ',';
                }
                child = child->next;
            }
            
            buf[(*p)++] = close_bracket;
        }
    }
    
    dump_cell(router->root, buffer, &pos);
    buffer[pos] = '\0';
    
    // Calculate checksum and format result
    uint16_t checksum = calculate_checksum(buffer);
    
    char* result = malloc(pos + 16);
    if (!result) return NULL;
    
    snprintf(result, pos + 16, "%04x,%s", checksum, buffer);
    
    return result;
}

int layout_zoom_pane(layout_router_t* router, layout_cell_t* cell) {
    if (!router) return -1;
    
    return zoom_pane_impl(router, cell);
}

int layout_unzoom_pane(layout_router_t* router) {
    if (!router) return -1;
    
    return unzoom_pane_impl(router);
}

bool layout_is_zoomed(layout_router_t* router) {
    return router && router->zoomed_pane != NULL;
}

int layout_sync_panes(layout_router_t* router, bool enable) {
    if (!router) return -1;
    
    router->sync_enabled = enable;
    
    // Mark all panes as synchronized
    void sync_recursive(layout_cell_t* cell) {
        if (!cell) return;
        
        if (cell->type == LAYOUT_WINDOWPANE) {
            cell->pane.synchronized = enable;
        } else {
            layout_cell_t* child = cell->children.first;
            while (child) {
                sync_recursive(child);
                child = child->next;
            }
        }
    }
    
    sync_recursive(router->root);
    
    return 0;
}

bool layout_are_synchronized(layout_router_t* router) {
    return router && router->sync_enabled;
}

layout_cell_t* layout_find_pane(layout_router_t* router, uint32_t id) {
    if (!router || !router->root) return NULL;
    
    return find_pane_recursive(router->root, id);
}

layout_cell_t* layout_find_adjacent(layout_router_t* router, layout_cell_t* cell,
                                    int direction) {
    if (!router || !cell) return NULL;
    
    return find_adjacent_impl(cell, direction);
}

layout_cell_t* layout_find_by_position(layout_router_t* router, uint32_t x, uint32_t y) {
    if (!router || !router->root) return NULL;
    
    return find_by_position_recursive(router->root, x, y);
}