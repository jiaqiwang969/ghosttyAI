/*
 * Grid Operations Implementation with SIMD
 * Task: T-202
 */

#include <stdlib.h>
#include <string.h>

#ifdef USE_AVX2
#include <immintrin.h>
#elif defined(USE_NEON)
#include <arm_neon.h>
#endif

#include "grid_callbacks.h"

/* Global backend instance */
static grid_ops_t grid_backend_impl;
static bool simd_enabled = false;

/* Grid creation */
static grid_t* grid_create_impl(int rows, int cols) {
    grid_t *grid = calloc(1, sizeof(grid_t));
    if (!grid) return NULL;
    
    grid->rows = rows;
    grid->cols = cols;
    
    /* Allocate cell array */
    grid->cells = calloc(rows, sizeof(grid_cell_t*));
    if (!grid->cells) {
        free(grid);
        return NULL;
    }
    
    for (int i = 0; i < rows; i++) {
        grid->cells[i] = calloc(cols, sizeof(grid_cell_t));
        if (!grid->cells[i]) {
            for (int j = 0; j < i; j++) {
                free(grid->cells[j]);
            }
            free(grid->cells);
            free(grid);
            return NULL;
        }
        
        /* Initialize with spaces */
        for (int j = 0; j < cols; j++) {
            grid->cells[i][j].codepoint = ' ';
            grid->cells[i][j].attr = 0;
            grid->cells[i][j].fg = 7;
            grid->cells[i][j].bg = 0;
        }
    }
    
    grid->simd_available = simd_enabled;
    grid->use_avx2 = simd_enabled;
    
    return grid;
}

/* Grid destruction */
static void grid_destroy_impl(grid_t *grid) {
    if (!grid) return;
    
    if (grid->cells) {
        for (int i = 0; i < grid->rows; i++) {
            free(grid->cells[i]);
        }
        free(grid->cells);
    }
    
    free(grid);
}

/* Cell operations */
static void grid_set_cell_impl(grid_t *grid, int row, int col, const grid_cell_t *cell) {
    if (!grid || !cell || row < 0 || row >= grid->rows || col < 0 || col >= grid->cols) {
        return;
    }
    
    grid->cells[row][col] = *cell;
    grid->dirty.needs_redraw = true;
}

static void grid_get_cell_impl(const grid_t *grid, int row, int col, grid_cell_t *cell) {
    if (!grid || !cell || row < 0 || row >= grid->rows || col < 0 || col >= grid->cols) {
        return;
    }
    
    *cell = grid->cells[row][col];
}

/* Batch update with SIMD optimization */
static void grid_batch_update_impl(grid_t *grid, int start_row, int start_col,
                                   const grid_cell_t *cells, size_t count) {
    if (!grid || !cells || start_row < 0 || start_row >= grid->rows) {
        return;
    }
    
    int col = start_col;
    int row = start_row;
    
    if (grid->use_avx2 && count >= 8) {
        /* Use AVX2 for batches of 8 or more cells */
        grid_batch_update_avx2(&grid->cells[row][col], count, cells);
    } else {
        /* Fallback to scalar copy */
        for (size_t i = 0; i < count; i++) {
            if (col >= grid->cols) {
                col = 0;
                row++;
                if (row >= grid->rows) break;
            }
            grid->cells[row][col] = cells[i];
            col++;
        }
    }
    
    grid->dirty.needs_redraw = true;
}

/* Batch clear */
static void grid_batch_clear_impl(grid_t *grid, int start_row, int start_col,
                                  int end_row, int end_col) {
    if (!grid) return;
    
    grid_cell_t empty = {.codepoint = ' ', .attr = 0, .fg = 7, .bg = 0};
    
    for (int row = start_row; row <= end_row && row < grid->rows; row++) {
        for (int col = start_col; col <= end_col && col < grid->cols; col++) {
            grid->cells[row][col] = empty;
        }
    }
    
    grid->dirty.needs_redraw = true;
}

/* Dirty tracking */
static void grid_mark_dirty_impl(grid_t *grid, int row, int col) {
    if (!grid) return;
    
    if (!grid->dirty.needs_redraw) {
        grid->dirty.start_row = row;
        grid->dirty.end_row = row;
        grid->dirty.start_col = col;
        grid->dirty.end_col = col;
    } else {
        if (row < grid->dirty.start_row) grid->dirty.start_row = row;
        if (row > grid->dirty.end_row) grid->dirty.end_row = row;
        if (col < grid->dirty.start_col) grid->dirty.start_col = col;
        if (col > grid->dirty.end_col) grid->dirty.end_col = col;
    }
    
    grid->dirty.needs_redraw = true;
}

static void grid_mark_region_dirty_impl(grid_t *grid, int start_row, int start_col,
                                        int end_row, int end_col) {
    if (!grid) return;
    
    grid_mark_dirty_impl(grid, start_row, start_col);
    grid_mark_dirty_impl(grid, end_row, end_col);
}

static bool grid_is_dirty_impl(const grid_t *grid) {
    return grid ? grid->dirty.needs_redraw : false;
}

static void grid_clear_dirty_impl(grid_t *grid) {
    if (!grid) return;
    grid->dirty.needs_redraw = false;
}

/* Initialize backend */
void grid_init_backend(bool use_simd) {
    simd_enabled = use_simd;
    
    grid_backend_impl.create = grid_create_impl;
    grid_backend_impl.destroy = grid_destroy_impl;
    grid_backend_impl.set_cell = grid_set_cell_impl;
    grid_backend_impl.get_cell = grid_get_cell_impl;
    grid_backend_impl.batch_update = grid_batch_update_impl;
    grid_backend_impl.batch_clear = grid_batch_clear_impl;
    grid_backend_impl.mark_dirty = grid_mark_dirty_impl;
    grid_backend_impl.mark_region_dirty = grid_mark_region_dirty_impl;
    grid_backend_impl.is_dirty = grid_is_dirty_impl;
    grid_backend_impl.clear_dirty = grid_clear_dirty_impl;
}

const grid_ops_t* grid_get_backend(void) {
    return &grid_backend_impl;
}