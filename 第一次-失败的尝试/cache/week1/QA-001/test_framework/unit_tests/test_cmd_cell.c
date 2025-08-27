// test_cmd_cell.c - Unit Test for cmd_cell Operation
// Author: QA-001 (Test Lead)
// Date: 2025-08-25
// Version: 1.0.0

#include "test_framework.h"
#include "unity.h"

// Test context
static test_context_t* ctx = NULL;
static mock_backend_t* backend = NULL;

void setUp(void) {
    ctx = test_context_create();
    backend = mock_backend_create(ctx);
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_NOT_NULL(backend);
}

void tearDown(void) {
    if (backend) {
        mock_backend_destroy(backend);
        backend = NULL;
    }
    if (ctx) {
        print_memory_report(ctx);
        print_performance_report(ctx);
        test_context_destroy(ctx);
        ctx = NULL;
    }
}

// Test single ASCII character update
void test_cmd_cell_ascii(void) {
    // Create test context with single ASCII character
    struct tty_ctx* tty_ctx = generate_test_context(10, 20, 1);
    TEST_ASSERT_NOT_NULL(tty_ctx);
    
    ui_cell_t* cell = (ui_cell_t*)tty_ctx->cell_data;
    cell->codepoint = 'A';
    cell->fg_rgb = 0xFFFFFF;
    cell->bg_rgb = 0x000000;
    cell->attrs = UI_ATTR_BOLD;
    cell->width = 1;
    
    // Call the command
    backend->base.ops->cmd_cell(&backend->base, tty_ctx);
    
    // Verify the call was made
    TEST_ASSERT_EQUAL_UINT32(1, backend->cmd_cell_calls);
    
    // Verify cell was updated
    TEST_ASSERT_EQUAL_UINT64(1, ctx->total_cells_updated);
    
    // Clean up
    test_free(tty_ctx->cell_data);
    test_free(tty_ctx);
    
    // Check for memory leaks
    TEST_ASSERT_NO_MEMORY_LEAKS(ctx);
}

// Test Unicode character update
void test_cmd_cell_unicode(void) {
    struct tty_ctx* tty_ctx = generate_test_context(5, 10, 1);
    TEST_ASSERT_NOT_NULL(tty_ctx);
    
    ui_cell_t* cell = (ui_cell_t*)tty_ctx->cell_data;
    cell->codepoint = 0x4F60;  // Chinese character '你'
    cell->fg_rgb = 0xFF0000;
    cell->bg_rgb = 0x00FF00;
    cell->attrs = UI_ATTR_ITALIC;
    cell->width = 2;  // Double-width character
    
    backend->base.ops->cmd_cell(&backend->base, tty_ctx);
    
    TEST_ASSERT_EQUAL_UINT32(1, backend->cmd_cell_calls);
    TEST_ASSERT_EQUAL_UINT64(1, ctx->total_cells_updated);
    
    test_free(tty_ctx->cell_data);
    test_free(tty_ctx);
    
    TEST_ASSERT_NO_MEMORY_LEAKS(ctx);
}

// Test emoji character update
void test_cmd_cell_emoji(void) {
    struct tty_ctx* tty_ctx = generate_test_context(0, 0, 1);
    TEST_ASSERT_NOT_NULL(tty_ctx);
    
    ui_cell_t* cell = (ui_cell_t*)tty_ctx->cell_data;
    cell->codepoint = 0x1F600;  // Grinning face emoji
    cell->fg_rgb = UI_COLOR_DEFAULT;
    cell->bg_rgb = UI_COLOR_DEFAULT;
    cell->attrs = 0;
    cell->width = 2;
    
    backend->base.ops->cmd_cell(&backend->base, tty_ctx);
    
    TEST_ASSERT_EQUAL_UINT32(1, backend->cmd_cell_calls);
    TEST_ASSERT_EQUAL_UINT64(1, ctx->total_cells_updated);
    
    test_free(tty_ctx->cell_data);
    test_free(tty_ctx);
    
    TEST_ASSERT_NO_MEMORY_LEAKS(ctx);
}

// Test cell with all attributes
void test_cmd_cell_all_attributes(void) {
    struct tty_ctx* tty_ctx = generate_test_context(12, 40, 1);
    TEST_ASSERT_NOT_NULL(tty_ctx);
    
    ui_cell_t* cell = (ui_cell_t*)tty_ctx->cell_data;
    cell->codepoint = 'X';
    cell->fg_rgb = 0xFFFFFF;
    cell->bg_rgb = 0xFF00FF;
    cell->attrs = UI_ATTR_BOLD | UI_ATTR_ITALIC | UI_ATTR_UNDERLINE |
                  UI_ATTR_REVERSE | UI_ATTR_BLINK | UI_ATTR_STRIKE;
    cell->width = 1;
    
    backend->base.ops->cmd_cell(&backend->base, tty_ctx);
    
    TEST_ASSERT_EQUAL_UINT32(1, backend->cmd_cell_calls);
    TEST_ASSERT_EQUAL_UINT64(1, ctx->total_cells_updated);
    
    test_free(tty_ctx->cell_data);
    test_free(tty_ctx);
    
    TEST_ASSERT_NO_MEMORY_LEAKS(ctx);
}

// Test NULL context handling
void test_cmd_cell_null_context(void) {
    // This should not crash
    backend->base.ops->cmd_cell(&backend->base, NULL);
    
    // Call count should still increment
    TEST_ASSERT_EQUAL_UINT32(1, backend->cmd_cell_calls);
    
    // But no cells should be updated
    TEST_ASSERT_EQUAL_UINT64(0, ctx->total_cells_updated);
    
    TEST_ASSERT_NO_MEMORY_LEAKS(ctx);
}

// Test boundary conditions (edge of screen)
void test_cmd_cell_boundary_conditions(void) {
    // Test top-left corner
    struct tty_ctx* tty_ctx1 = generate_test_context(0, 0, 1);
    backend->base.ops->cmd_cell(&backend->base, tty_ctx1);
    TEST_ASSERT_EQUAL_UINT32(1, backend->cmd_cell_calls);
    
    // Test bottom-right corner (assuming 80x24 terminal)
    struct tty_ctx* tty_ctx2 = generate_test_context(23, 79, 1);
    backend->base.ops->cmd_cell(&backend->base, tty_ctx2);
    TEST_ASSERT_EQUAL_UINT32(2, backend->cmd_cell_calls);
    
    // Test out of bounds (should be handled gracefully)
    struct tty_ctx* tty_ctx3 = generate_test_context(100, 200, 1);
    backend->base.ops->cmd_cell(&backend->base, tty_ctx3);
    TEST_ASSERT_EQUAL_UINT32(3, backend->cmd_cell_calls);
    
    test_free(tty_ctx1->cell_data);
    test_free(tty_ctx1);
    test_free(tty_ctx2->cell_data);
    test_free(tty_ctx2);
    test_free(tty_ctx3->cell_data);
    test_free(tty_ctx3);
    
    TEST_ASSERT_NO_MEMORY_LEAKS(ctx);
}

// Test rapid successive updates (performance)
void test_cmd_cell_performance(void) {
    uint64_t start_ns = get_time_ns();
    
    // Send 10000 cell updates
    for (int i = 0; i < 10000; i++) {
        struct tty_ctx* tty_ctx = generate_test_context(
            i % 24,  // Row
            i % 80,  // Column
            1        // Single cell
        );
        
        backend->base.ops->cmd_cell(&backend->base, tty_ctx);
        
        test_free(tty_ctx->cell_data);
        test_free(tty_ctx);
    }
    
    uint64_t end_ns = get_time_ns();
    double elapsed_ms = (end_ns - start_ns) / 1000000.0;
    
    // Verify all calls were made
    TEST_ASSERT_EQUAL_UINT32(10000, backend->cmd_cell_calls);
    TEST_ASSERT_EQUAL_UINT64(10000, ctx->total_cells_updated);
    
    // Check performance (should handle 10000 cells quickly)
    TEST_ASSERT_TRUE_MESSAGE(elapsed_ms < 100, "Performance too slow");
    
    printf("Performance: 10000 cells in %.2f ms (%.0f cells/sec)\n",
           elapsed_ms, 10000000.0 / elapsed_ms);
    
    TEST_ASSERT_NO_MEMORY_LEAKS(ctx);
}

// Test grapheme cluster handling
void test_cmd_cell_grapheme_cluster(void) {
    struct tty_ctx* tty_ctx = generate_test_context(10, 20, 1);
    TEST_ASSERT_NOT_NULL(tty_ctx);
    
    ui_cell_t* cell = (ui_cell_t*)tty_ctx->cell_data;
    cell->codepoint = 0x0041;  // 'A'
    cell->fg_rgb = 0xFFFFFF;
    cell->bg_rgb = 0x000000;
    cell->attrs = 0;
    cell->width = 1;
    cell->cluster_cont = 1;  // Part of a grapheme cluster
    
    backend->base.ops->cmd_cell(&backend->base, tty_ctx);
    
    TEST_ASSERT_EQUAL_UINT32(1, backend->cmd_cell_calls);
    
    test_free(tty_ctx->cell_data);
    test_free(tty_ctx);
    
    TEST_ASSERT_NO_MEMORY_LEAKS(ctx);
}

// Test zero-width characters
void test_cmd_cell_zero_width(void) {
    struct tty_ctx* tty_ctx = generate_test_context(5, 10, 1);
    TEST_ASSERT_NOT_NULL(tty_ctx);
    
    ui_cell_t* cell = (ui_cell_t*)tty_ctx->cell_data;
    cell->codepoint = 0x0301;  // Combining acute accent
    cell->fg_rgb = 0xFFFFFF;
    cell->bg_rgb = 0x000000;
    cell->attrs = 0;
    cell->width = 0;  // Zero-width combining character
    
    backend->base.ops->cmd_cell(&backend->base, tty_ctx);
    
    TEST_ASSERT_EQUAL_UINT32(1, backend->cmd_cell_calls);
    
    test_free(tty_ctx->cell_data);
    test_free(tty_ctx);
    
    TEST_ASSERT_NO_MEMORY_LEAKS(ctx);
}

// Test color edge cases
void test_cmd_cell_color_edge_cases(void) {
    struct tty_ctx* tty_ctx = generate_test_context(10, 20, 1);
    TEST_ASSERT_NOT_NULL(tty_ctx);
    
    ui_cell_t* cell = (ui_cell_t*)tty_ctx->cell_data;
    
    // Test with default colors
    cell->codepoint = 'D';
    cell->fg_rgb = UI_COLOR_DEFAULT;
    cell->bg_rgb = UI_COLOR_DEFAULT;
    backend->base.ops->cmd_cell(&backend->base, tty_ctx);
    
    // Test with invalid colors
    cell->fg_rgb = UI_COLOR_INVALID;
    cell->bg_rgb = UI_COLOR_INVALID;
    backend->base.ops->cmd_cell(&backend->base, tty_ctx);
    
    // Test with 24-bit RGB colors
    cell->fg_rgb = 0x123456;
    cell->bg_rgb = 0xFEDCBA;
    backend->base.ops->cmd_cell(&backend->base, tty_ctx);
    
    TEST_ASSERT_EQUAL_UINT32(3, backend->cmd_cell_calls);
    
    test_free(tty_ctx->cell_data);
    test_free(tty_ctx);
    
    TEST_ASSERT_NO_MEMORY_LEAKS(ctx);
}

// Main test runner
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_cmd_cell_ascii);
    RUN_TEST(test_cmd_cell_unicode);
    RUN_TEST(test_cmd_cell_emoji);
    RUN_TEST(test_cmd_cell_all_attributes);
    RUN_TEST(test_cmd_cell_null_context);
    RUN_TEST(test_cmd_cell_boundary_conditions);
    RUN_TEST(test_cmd_cell_performance);
    RUN_TEST(test_cmd_cell_grapheme_cluster);
    RUN_TEST(test_cmd_cell_zero_width);
    RUN_TEST(test_cmd_cell_color_edge_cases);
    
    return UNITY_END();
}