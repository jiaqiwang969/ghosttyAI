// frame_validator.c - Frame Validation Utilities
// Author: QA-001 (Test Lead)
// Date: 2025-08-25
// Version: 1.0.0

#include "test_framework.h"
#include <math.h>

// ============================================================================
// Frame Statistics
// ============================================================================

typedef struct frame_stats {
    uint32_t total_frames;
    uint32_t valid_frames;
    uint32_t invalid_frames;
    uint32_t dropped_frames;
    uint32_t urgent_frames;
    uint32_t complete_frames;
    uint32_t partial_frames;
    
    double avg_span_count;
    double avg_cells_per_frame;
    double avg_interval_ms;
    
    uint32_t max_spans_per_frame;
    uint32_t max_cells_per_frame;
    uint64_t max_interval_ns;
    uint64_t min_interval_ns;
} frame_stats_t;

// Calculate frame statistics
frame_stats_t* calculate_frame_stats(test_context_t* ctx) {
    if (!ctx || ctx->frame_count == 0) return NULL;
    
    frame_stats_t* stats = test_malloc(sizeof(frame_stats_t));
    if (!stats) return NULL;
    
    memset(stats, 0, sizeof(frame_stats_t));
    stats->total_frames = ctx->frame_count;
    stats->min_interval_ns = UINT64_MAX;
    
    uint64_t total_spans = 0;
    uint64_t total_cells = 0;
    uint64_t total_interval = 0;
    
    for (uint32_t i = 0; i < ctx->frame_count; i++) {
        ui_frame_t* frame = ctx->captured_frames[i];
        if (!frame) continue;
        
        // Validate frame
        if (validate_frame(frame)) {
            stats->valid_frames++;
        } else {
            stats->invalid_frames++;
        }
        
        // Count frame types
        if (frame->flags & UI_FRAME_DROPPED) {
            stats->dropped_frames += frame->frames_dropped;
        }
        if (frame->flags & UI_FRAME_URGENT) {
            stats->urgent_frames++;
        }
        if (frame->flags & UI_FRAME_COMPLETE) {
            stats->complete_frames++;
        }
        if (frame->flags & UI_FRAME_PARTIAL) {
            stats->partial_frames++;
        }
        
        // Track spans and cells
        total_spans += frame->span_count;
        total_cells += frame->cells_modified;
        
        if (frame->span_count > stats->max_spans_per_frame) {
            stats->max_spans_per_frame = frame->span_count;
        }
        if (frame->cells_modified > stats->max_cells_per_frame) {
            stats->max_cells_per_frame = frame->cells_modified;
        }
        
        // Track intervals
        if (i > 0) {
            uint64_t interval = ctx->frame_timestamps[i] - ctx->frame_timestamps[i-1];
            total_interval += interval;
            
            if (interval > stats->max_interval_ns) {
                stats->max_interval_ns = interval;
            }
            if (interval < stats->min_interval_ns) {
                stats->min_interval_ns = interval;
            }
        }
    }
    
    // Calculate averages
    if (stats->total_frames > 0) {
        stats->avg_span_count = (double)total_spans / stats->total_frames;
        stats->avg_cells_per_frame = (double)total_cells / stats->total_frames;
    }
    
    if (stats->total_frames > 1) {
        stats->avg_interval_ms = (total_interval / (stats->total_frames - 1)) / 1000000.0;
    }
    
    return stats;
}

// Print frame statistics report
void print_frame_stats(frame_stats_t* stats) {
    if (!stats) return;
    
    printf("\n=== Frame Statistics ===\n");
    printf("Total frames: %u\n", stats->total_frames);
    printf("  Valid: %u (%.1f%%)\n", stats->valid_frames,
           100.0 * stats->valid_frames / stats->total_frames);
    printf("  Invalid: %u\n", stats->invalid_frames);
    printf("  Dropped: %u\n", stats->dropped_frames);
    printf("  Urgent: %u\n", stats->urgent_frames);
    printf("  Complete: %u\n", stats->complete_frames);
    printf("  Partial: %u\n", stats->partial_frames);
    
    printf("\nFrame content:\n");
    printf("  Avg spans/frame: %.1f\n", stats->avg_span_count);
    printf("  Avg cells/frame: %.1f\n", stats->avg_cells_per_frame);
    printf("  Max spans/frame: %u\n", stats->max_spans_per_frame);
    printf("  Max cells/frame: %u\n", stats->max_cells_per_frame);
    
    printf("\nFrame timing:\n");
    printf("  Avg interval: %.2f ms\n", stats->avg_interval_ms);
    printf("  Min interval: %.2f ms\n", stats->min_interval_ns / 1000000.0);
    printf("  Max interval: %.2f ms\n", stats->max_interval_ns / 1000000.0);
    
    // Check for issues
    if (stats->avg_interval_ms > 20.0) {
        printf("  WARNING: Average interval exceeds 20ms (laggy)\n");
    }
    if (stats->max_interval_ns > 33333333) {  // 33.33ms = 30 FPS
        printf("  WARNING: Max interval exceeds 33ms (< 30 FPS)\n");
    }
}

// ============================================================================
// Advanced Frame Validation
// ============================================================================

// Validate frame contains expected content
bool validate_frame_content(const ui_frame_t* frame, 
                           uint32_t expected_row,
                           uint32_t expected_col,
                           const char* expected_text) {
    if (!frame || !expected_text) return false;
    
    size_t text_len = strlen(expected_text);
    
    // Search for the text in the frame spans
    for (uint32_t i = 0; i < frame->span_count; i++) {
        const ui_span_t* span = &frame->spans[i];
        
        if (span->row == expected_row &&
            span->col_start <= expected_col &&
            span->col_end > expected_col) {
            
            // Check if text matches
            uint32_t offset = expected_col - span->col_start;
            uint32_t available = span->col_end - expected_col;
            
            if (available >= text_len && span->cells) {
                bool matches = true;
                for (size_t j = 0; j < text_len; j++) {
                    if (span->cells[offset + j].codepoint != expected_text[j]) {
                        matches = false;
                        break;
                    }
                }
                if (matches) return true;
            }
        }
    }
    
    return false;
}

// Validate frame represents a scroll operation
bool validate_frame_scroll(const ui_frame_t* frame, int32_t lines) {
    if (!frame) return false;
    
    // For scroll operations, we expect:
    // 1. Multiple spans affected
    // 2. Systematic row changes
    // 3. Possible new content at top/bottom
    
    if (frame->span_count < abs(lines)) {
        return false;  // Too few spans for scroll
    }
    
    // Check if spans show systematic movement
    // This is simplified; real validation would be more complex
    return true;
}

// Validate frame represents a clear operation
bool validate_frame_clear(const ui_frame_t* frame,
                         uint32_t start_row, uint32_t end_row,
                         uint32_t start_col, uint32_t end_col) {
    if (!frame) return false;
    
    // For clear operations, check all affected cells are spaces
    for (uint32_t i = 0; i < frame->span_count; i++) {
        const ui_span_t* span = &frame->spans[i];
        
        if (span->row >= start_row && span->row <= end_row) {
            for (uint32_t j = 0; j < (span->col_end - span->col_start); j++) {
                if (span->cells && span->cells[j].codepoint != ' ') {
                    return false;  // Non-space found in cleared area
                }
            }
        }
    }
    
    return true;
}

// ============================================================================
// Frame Sequence Validation
// ============================================================================

// Validate synchronized update sequence
bool validate_sync_sequence(test_context_t* ctx) {
    if (!ctx || ctx->frame_count < 2) return true;
    
    bool in_sync = false;
    uint32_t sync_start_frame = 0;
    
    for (uint32_t i = 0; i < ctx->frame_count; i++) {
        ui_frame_t* frame = ctx->captured_frames[i];
        if (!frame) continue;
        
        // Check for sync start
        if (!in_sync && (frame->flags & UI_FRAME_PARTIAL)) {
            in_sync = true;
            sync_start_frame = i;
        }
        
        // Check for sync end
        if (in_sync && (frame->flags & UI_FRAME_COMPLETE)) {
            in_sync = false;
            
            // Validate sync block
            uint32_t sync_frames = i - sync_start_frame + 1;
            if (sync_frames > 100) {
                printf("WARNING: Sync block too large: %u frames\n", sync_frames);
                return false;
            }
        }
    }
    
    // Should not end while still in sync
    if (in_sync) {
        printf("ERROR: Sync block not completed\n");
        return false;
    }
    
    return true;
}

// Validate frame rate consistency
bool validate_frame_rate(test_context_t* ctx, double target_fps, double tolerance) {
    if (!ctx || ctx->frame_count < 10) return true;
    
    uint64_t target_interval_ns = 1000000000.0 / target_fps;
    uint64_t tolerance_ns = target_interval_ns * tolerance;
    
    uint32_t violations = 0;
    
    for (uint32_t i = 1; i < ctx->frame_count; i++) {
        uint64_t interval = ctx->frame_timestamps[i] - ctx->frame_timestamps[i-1];
        
        if (interval < target_interval_ns - tolerance_ns ||
            interval > target_interval_ns + tolerance_ns) {
            violations++;
        }
    }
    
    double violation_rate = (double)violations / (ctx->frame_count - 1);
    
    if (violation_rate > 0.1) {  // More than 10% violations
        printf("WARNING: Frame rate inconsistent - %.1f%% violations\n",
               violation_rate * 100);
        return false;
    }
    
    return true;
}

// ============================================================================
// Frame Comparison
// ============================================================================

// Compare two frames for equality
bool frames_equal(const ui_frame_t* frame1, const ui_frame_t* frame2) {
    if (!frame1 || !frame2) return frame1 == frame2;
    
    if (frame1->span_count != frame2->span_count ||
        frame1->cells_modified != frame2->cells_modified ||
        frame1->pane_id != frame2->pane_id) {
        return false;
    }
    
    // Compare spans
    for (uint32_t i = 0; i < frame1->span_count; i++) {
        const ui_span_t* span1 = &frame1->spans[i];
        const ui_span_t* span2 = &frame2->spans[i];
        
        if (span1->row != span2->row ||
            span1->col_start != span2->col_start ||
            span1->col_end != span2->col_end) {
            return false;
        }
        
        // Compare cells
        uint32_t cell_count = span1->col_end - span1->col_start;
        for (uint32_t j = 0; j < cell_count; j++) {
            const ui_cell_t* cell1 = &span1->cells[j];
            const ui_cell_t* cell2 = &span2->cells[j];
            
            if (cell1->codepoint != cell2->codepoint ||
                cell1->fg_rgb != cell2->fg_rgb ||
                cell1->bg_rgb != cell2->bg_rgb ||
                cell1->attrs != cell2->attrs) {
                return false;
            }
        }
    }
    
    return true;
}

// Calculate frame difference
uint32_t frame_diff_cells(const ui_frame_t* frame1, const ui_frame_t* frame2) {
    if (!frame1 || !frame2) return UINT32_MAX;
    
    uint32_t diff_count = 0;
    
    // Simple implementation: count all cells as different if frames differ
    // A real implementation would do cell-by-cell comparison
    if (!frames_equal(frame1, frame2)) {
        diff_count = frame1->cells_modified + frame2->cells_modified;
    }
    
    return diff_count;
}