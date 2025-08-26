/*
 * Simple Integration Test
 * Quick verification that code compiles and runs
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "event_loop_backend.h"
#include "grid_callbacks.h"
#include "layout_callbacks.h"

int main() {
    printf("=================================================\n");
    printf("Ghostty × tmux Integration - Quick Test\n");
    printf("=================================================\n\n");
    
    // Test 1: Event Loop
    printf("Testing Event Loop... ");
    struct event_base *base = event_base_new();
    assert(base != NULL);
    
    struct event ev;
    event_set(&ev, -1, EV_TIMEOUT, NULL, NULL);
    assert(event_add(&ev, NULL) == 0);
    assert(event_del(&ev) == 0);
    
    event_base_free(base);
    printf("PASS\n");
    
    // Test 2: Grid Operations
    printf("Testing Grid Operations... ");
    grid_init_backend(true);
    const grid_ops_t *ops = grid_get_backend();
    assert(ops != NULL);
    
    grid_t *grid = ops->create(24, 80);
    assert(grid != NULL);
    
    grid_cell_t cell = {.codepoint = 'A', .attr = 0, .fg = 7, .bg = 0};
    ops->set_cell(grid, 0, 0, &cell);
    
    grid_cell_t retrieved;
    ops->get_cell(grid, 0, 0, &retrieved);
    assert(retrieved.codepoint == 'A');
    
    ops->destroy(grid);
    printf("PASS\n");
    
    // Test 3: Layout Manager
    printf("Testing Layout Manager... ");
    layout_manager_t *mgr = layout_manager_create();
    assert(mgr != NULL);
    assert(mgr->ops != NULL);
    
    layout_manager_destroy(mgr);
    printf("PASS\n");
    
    printf("\n✅ All basic tests passed!\n");
    printf("=================================================\n");
    
    return 0;
}