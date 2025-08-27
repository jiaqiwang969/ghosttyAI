#!/bin/bash
# analyze_performance.sh - Analyze performance benchmark results
# Author: QA-001 (Test Lead)
# Date: 2025-08-25

REPORTS_DIR="reports/performance"
PERF_TARGET_CELLS_SEC=10000000
PERF_TARGET_FPS=60
PERF_TARGET_LATENCY_MS=16.67

echo "========================================"
echo "Performance Analysis Report"
echo "========================================"
echo "Timestamp: $(date)"
echo ""

# Check if performance reports exist
if [ ! -d "$REPORTS_DIR" ]; then
    echo "ERROR: Performance reports directory not found: $REPORTS_DIR"
    exit 1
fi

# Analyze cell update performance
if [ -f "$REPORTS_DIR/bench_cell_updates.bin.txt" ]; then
    echo "Cell Update Performance:"
    echo "------------------------"
    CELLS_PER_SEC=$(grep "cells/sec" "$REPORTS_DIR/bench_cell_updates.bin.txt" | awk '{print $NF}')
    if [ -n "$CELLS_PER_SEC" ]; then
        echo "  Measured: $CELLS_PER_SEC cells/sec"
        echo "  Target: $PERF_TARGET_CELLS_SEC cells/sec"
        if (( $(echo "$CELLS_PER_SEC < $PERF_TARGET_CELLS_SEC" | bc -l) )); then
            echo "  Status: FAIL - Below target"
        else
            echo "  Status: PASS"
        fi
    fi
    echo ""
fi

# Analyze frame rate performance
if [ -f "$REPORTS_DIR/bench_60fps_rendering.bin.txt" ]; then
    echo "Frame Rate Performance:"
    echo "----------------------"
    FPS=$(grep "FPS" "$REPORTS_DIR/bench_60fps_rendering.bin.txt" | awk '{print $NF}')
    if [ -n "$FPS" ]; then
        echo "  Measured: $FPS FPS"
        echo "  Target: $PERF_TARGET_FPS FPS"
        if (( $(echo "$FPS < $PERF_TARGET_FPS" | bc -l) )); then
            echo "  Status: FAIL - Below target"
        else
            echo "  Status: PASS"
        fi
    fi
    echo ""
fi

# Analyze latency
if [ -f "$REPORTS_DIR/bench_frame_aggregation.bin.txt" ]; then
    echo "Latency Performance:"
    echo "-------------------"
    P50_LATENCY=$(grep "P50" "$REPORTS_DIR/bench_frame_aggregation.bin.txt" | awk '{print $NF}')
    P95_LATENCY=$(grep "P95" "$REPORTS_DIR/bench_frame_aggregation.bin.txt" | awk '{print $NF}')
    P99_LATENCY=$(grep "P99" "$REPORTS_DIR/bench_frame_aggregation.bin.txt" | awk '{print $NF}')
    
    if [ -n "$P95_LATENCY" ]; then
        echo "  P50 Latency: $P50_LATENCY ms"
        echo "  P95 Latency: $P95_LATENCY ms"
        echo "  P99 Latency: $P99_LATENCY ms"
        echo "  Target: < $PERF_TARGET_LATENCY_MS ms"
        
        if (( $(echo "$P95_LATENCY > $PERF_TARGET_LATENCY_MS" | bc -l) )); then
            echo "  Status: FAIL - P95 latency too high"
        else
            echo "  Status: PASS"
        fi
    fi
    echo ""
fi

# Memory usage analysis
echo "Memory Usage Analysis:"
echo "---------------------"
for report in "$REPORTS_DIR"/*.txt; do
    if grep -q "Maximum resident set size" "$report"; then
        MEM_KB=$(grep "Maximum resident set size" "$report" | awk '{print $NF}')
        MEM_MB=$((MEM_KB / 1024))
        echo "  $(basename "$report"): ${MEM_MB} MB"
    fi
done
echo ""

# CPU usage analysis
echo "CPU Usage Analysis:"
echo "------------------"
for report in "$REPORTS_DIR"/*.txt; do
    if grep -q "Percent of CPU" "$report"; then
        CPU_PERCENT=$(grep "Percent of CPU" "$report" | awk '{print $NF}')
        echo "  $(basename "$report"): ${CPU_PERCENT}"
    fi
done
echo ""

# Summary
echo "========================================"
echo "Performance Summary"
echo "========================================"

PASSED=0
FAILED=0

# Check all targets
if [ -n "$CELLS_PER_SEC" ] && (( $(echo "$CELLS_PER_SEC >= $PERF_TARGET_CELLS_SEC" | bc -l) )); then
    PASSED=$((PASSED + 1))
else
    FAILED=$((FAILED + 1))
fi

if [ -n "$FPS" ] && (( $(echo "$FPS >= $PERF_TARGET_FPS" | bc -l) )); then
    PASSED=$((PASSED + 1))
else
    FAILED=$((FAILED + 1))
fi

if [ -n "$P95_LATENCY" ] && (( $(echo "$P95_LATENCY <= $PERF_TARGET_LATENCY_MS" | bc -l) )); then
    PASSED=$((PASSED + 1))
else
    FAILED=$((FAILED + 1))
fi

echo "Tests Passed: $PASSED"
echo "Tests Failed: $FAILED"

if [ $FAILED -eq 0 ]; then
    echo "Result: ALL PERFORMANCE TARGETS MET"
    exit 0
else
    echo "Result: PERFORMANCE TARGETS NOT MET"
    exit 1
fi