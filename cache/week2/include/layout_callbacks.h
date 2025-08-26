/*
 * Layout Management Callbacks
 * Task: T-203 - 布局管理callbacks
 * Performance: <50ms layout switching
 */

#ifndef LAYOUT_CALLBACKS_H
#define LAYOUT_CALLBACKS_H

#include <stdint.h>
#include <stdbool.h>

/* Layout types matching tmux */
typedef enum {
    LAYOUT_EVEN_HORIZONTAL,
    LAYOUT_EVEN_VERTICAL,
    LAYOUT_MAIN_HORIZONTAL,
    LAYOUT_MAIN_VERTICAL,
    LAYOUT_TILED,
    LAYOUT_CUSTOM
} layout_type_t;

/* Pane structure */
typedef struct pane {
    int id;
    int x, y;
    int width, height;
    
    /* Pane relationships */
    struct pane *parent;
    struct pane *next;
    struct pane *prev;
    
    /* Content association */
    void *window;  /* Associated window */
    void *data;    /* User data */
    
    /* Flags */
    bool active;
    bool zoomed;
    bool marked;
} pane_t;

/* Layout cell for custom layouts */
typedef struct layout_cell {
    enum {
        LAYOUT_CELL_PANE,
        LAYOUT_CELL_HORIZONTAL,
        LAYOUT_CELL_VERTICAL
    } type;
    
    int x, y;
    int width, height;
    
    union {
        pane_t *pane;
        struct {
            struct layout_cell *first;
            struct layout_cell *last;
        } children;
    };
    
    struct layout_cell *parent;
    struct layout_cell *next;
} layout_cell_t;

/* Layout operations callbacks */
typedef struct layout_ops {
    /* Layout management */
    int (*set_layout)(layout_type_t type);
    int (*apply_custom_layout)(const char *layout_string);
    char* (*get_layout_string)(void);
    
    /* Pane operations - <10ms response time */
    pane_t* (*split_pane)(pane_t *pane, bool vertical, int size);
    int (*close_pane)(pane_t *pane);
    int (*resize_pane)(pane_t *pane, int dx, int dy);
    int (*swap_panes)(pane_t *pane1, pane_t *pane2);
    
    /* Pane selection */
    pane_t* (*select_pane)(int direction);  /* UP, DOWN, LEFT, RIGHT */
    pane_t* (*get_active_pane)(void);
    pane_t* (*find_pane)(int id);
    
    /* Layout calculations */
    void (*recalculate_layout)(void);
    void (*balance_panes)(void);
    
    /* Zoom operations */
    int (*zoom_pane)(pane_t *pane);
    int (*unzoom_pane)(void);
    bool (*is_zoomed)(void);
    
    /* Events - callbacks from layout changes */
    void (*on_layout_change)(layout_type_t new_layout);
    void (*on_pane_create)(pane_t *pane);
    void (*on_pane_close)(pane_t *pane);
    void (*on_pane_resize)(pane_t *pane);
    
    /* Serialization */
    char* (*serialize_layout)(void);
    int (*deserialize_layout)(const char *data);
    
    /* Performance stats */
    uint64_t (*get_switch_time_us)(void);
    uint64_t (*get_resize_time_us)(void);
} layout_ops_t;

/* Layout manager state */
typedef struct layout_manager {
    layout_type_t current_layout;
    layout_cell_t *root_cell;
    pane_t *pane_list;
    pane_t *active_pane;
    
    /* Layout operations */
    const layout_ops_t *ops;
    
    /* Performance tracking */
    uint64_t layout_switches;
    uint64_t total_switch_time_us;
    uint64_t pane_operations;
} layout_manager_t;

/* Global layout manager */
extern layout_manager_t* layout_mgr;

/* Initialization */
layout_manager_t* layout_manager_create(void);
void layout_manager_destroy(layout_manager_t *mgr);

/* Fast layout algorithms */

/* Even split - O(n) time complexity */
static inline void layout_even_split(pane_t *panes, int count, 
                                     int width, int height, bool vertical) {
    if (count == 0) return;
    
    int size = vertical ? (height / count) : (width / count);
    int remainder = vertical ? (height % count) : (width % count);
    
    pane_t *pane = panes;
    int offset = 0;
    
    for (int i = 0; i < count; i++, pane = pane->next) {
        if (vertical) {
            pane->x = 0;
            pane->y = offset;
            pane->width = width;
            pane->height = size + (i < remainder ? 1 : 0);
            offset += pane->height;
        } else {
            pane->x = offset;
            pane->y = 0;
            pane->width = size + (i < remainder ? 1 : 0);
            pane->height = height;
            offset += pane->width;
        }
    }
}

/* Main layout with configurable main pane size */
static inline void layout_main_split(pane_t *panes, int count,
                                     int width, int height, 
                                     bool vertical, int main_size_percent) {
    if (count == 0) return;
    
    pane_t *main_pane = panes;
    int main_size = vertical ? 
        (height * main_size_percent / 100) :
        (width * main_size_percent / 100);
    
    /* Set main pane */
    if (vertical) {
        main_pane->x = 0;
        main_pane->y = 0;
        main_pane->width = width;
        main_pane->height = main_size;
    } else {
        main_pane->x = 0;
        main_pane->y = 0;
        main_pane->width = main_size;
        main_pane->height = height;
    }
    
    /* Distribute remaining panes */
    if (count > 1) {
        int remaining = vertical ? (height - main_size) : (width - main_size);
        int offset = main_size;
        int size = remaining / (count - 1);
        
        pane_t *pane = panes->next;
        for (int i = 1; i < count; i++, pane = pane->next) {
            if (vertical) {
                pane->x = 0;
                pane->y = offset;
                pane->width = width;
                pane->height = (i == count - 1) ? remaining : size;
            } else {
                pane->x = offset;
                pane->y = 0;
                pane->width = (i == count - 1) ? remaining : size;
                pane->height = height;
            }
            offset += size;
        }
    }
}

/* Tiled layout - optimized grid arrangement */
static inline void layout_tiled(pane_t *panes, int count,
                                int width, int height) {
    if (count == 0) return;
    
    /* Calculate optimal grid dimensions */
    int cols = 1;
    while (cols * cols < count) cols++;
    int rows = (count + cols - 1) / cols;
    
    int cell_width = width / cols;
    int cell_height = height / rows;
    
    pane_t *pane = panes;
    for (int i = 0; i < count; i++, pane = pane->next) {
        int row = i / cols;
        int col = i % cols;
        
        pane->x = col * cell_width;
        pane->y = row * cell_height;
        pane->width = (col == cols - 1) ? width - pane->x : cell_width;
        pane->height = (row == rows - 1) ? height - pane->y : cell_height;
    }
}

#endif /* LAYOUT_CALLBACKS_H */