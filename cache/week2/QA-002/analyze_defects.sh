#!/bin/bash
# Defect Analysis Script for Week 2 Components
# QA-002 Task T-404

CACHE_DIR="/Users/jqwang/98-ghosttyAI/cache/week2"
REPORT_DIR="$CACHE_DIR/QA-002/reports"
DEFECTS_DIR="$CACHE_DIR/QA-002/defects"

echo "=== Starting Defect Analysis for Week 2 Components ==="
echo "Time: $(date)"
echo ""

# Create analysis report
REPORT="$REPORT_DIR/static_analysis_report.md"
cat > "$REPORT" << 'EOF'
# Static Analysis Report
**Date**: 2025-08-26
**Task**: T-404 Defect Fixes

## Analysis Summary

EOF

# Function to analyze C files
analyze_c_files() {
    local component=$1
    local dir=$2
    
    echo "## $component" >> "$REPORT"
    echo "" >> "$REPORT"
    
    # Check if directory exists
    if [ ! -d "$dir" ]; then
        echo "Directory not found: $dir" >> "$REPORT"
        return
    fi
    
    # Find all C files
    find "$dir" -name "*.c" -o -name "*.h" | while read -r file; do
        echo "Analyzing: $file"
        echo "### $(basename $file)" >> "$REPORT"
        echo "" >> "$REPORT"
        
        # Check for common issues
        echo "#### Potential Issues:" >> "$REPORT"
        
        # NULL pointer checks
        if grep -n "->.*=" "$file" | grep -v "if.*!=" | grep -v "assert" > /dev/null; then
            echo "- **P1**: Missing NULL pointer checks found" >> "$REPORT"
            grep -n "->.*=" "$file" | grep -v "if.*!=" | grep -v "assert" | head -3 >> "$REPORT"
            echo "" >> "$REPORT"
        fi
        
        # Memory alignment for SIMD
        if grep -q "__m256" "$file"; then
            if ! grep -q "aligned_alloc\|_aligned_malloc\|__attribute__((aligned" "$file"; then
                echo "- **P0**: SIMD operations without proper alignment" >> "$REPORT"
            fi
        fi
        
        # Buffer overflow risks
        if grep -q "strcpy\|strcat\|sprintf\|gets" "$file"; then
            echo "- **P0**: Unsafe string functions detected" >> "$REPORT"
            grep -n "strcpy\|strcat\|sprintf\|gets" "$file" >> "$REPORT"
            echo "" >> "$REPORT"
        fi
        
        # Resource leaks
        if grep -q "malloc\|calloc\|realloc" "$file"; then
            malloc_count=$(grep -c "malloc\|calloc\|realloc" "$file")
            free_count=$(grep -c "free(" "$file")
            if [ "$malloc_count" -gt "$free_count" ]; then
                echo "- **P1**: Potential memory leak (malloc:$malloc_count, free:$free_count)" >> "$REPORT"
            fi
        fi
        
        # Race conditions
        if grep -q "pthread_mutex\|pthread_rwlock" "$file"; then
            if ! grep -q "pthread_mutex_lock\|pthread_rwlock_rdlock\|pthread_rwlock_wrlock" "$file"; then
                echo "- **P0**: Mutex declared but not used - potential race condition" >> "$REPORT"
            fi
        fi
        
        # Integer overflow
        if grep -n "\*.*\*\|<<.*[0-9][0-9]" "$file" | grep -v "comment" > /dev/null; then
            echo "- **P2**: Potential integer overflow in calculations" >> "$REPORT"
        fi
        
        echo "" >> "$REPORT"
    done
}

# Function to analyze Zig files
analyze_zig_files() {
    local component=$1
    local dir=$2
    
    echo "## $component" >> "$REPORT"
    echo "" >> "$REPORT"
    
    if [ ! -d "$dir" ]; then
        echo "Directory not found: $dir" >> "$REPORT"
        return
    fi
    
    find "$dir" -name "*.zig" | while read -r file; do
        echo "Analyzing: $file"
        echo "### $(basename $file)" >> "$REPORT"
        echo "" >> "$REPORT"
        
        echo "#### Potential Issues:" >> "$REPORT"
        
        # Check for unsafe operations
        if grep -q "@ptrCast\|@intToPtr\|@ptrToInt" "$file"; then
            echo "- **P1**: Unsafe pointer operations detected" >> "$REPORT"
            grep -n "@ptrCast\|@intToPtr\|@ptrToInt" "$file" | head -3 >> "$REPORT"
            echo "" >> "$REPORT"
        fi
        
        # Memory safety
        if grep -q "undefined" "$file" | grep -v "// " > /dev/null; then
            echo "- **P2**: Use of undefined values" >> "$REPORT"
        fi
        
        # Error handling
        if grep -q "try" "$file"; then
            if ! grep -q "catch\|errdefer" "$file"; then
                echo "- **P2**: Try without proper error handling" >> "$REPORT"
            fi
        fi
        
        echo "" >> "$REPORT"
    done
}

# Analyze each component
echo "Analyzing CORE-001 Event Loop..."
analyze_c_files "Event Loop Backend (T-201)" "$CACHE_DIR/CORE-001/src"

echo "Analyzing CORE-002 Grid Operations..."
analyze_c_files "Grid Operations (T-202)" "$CACHE_DIR/CORE-002/src"

echo "Analyzing INTG-001 FFI..."
analyze_zig_files "FFI Types (T-301)" "$CACHE_DIR/INTG-001/ffi"
analyze_zig_files "Ghostty Integration (T-302)" "$CACHE_DIR/INTG-001"

echo "Analyzing Layout Manager..."
analyze_c_files "Layout Manager (T-203)" "$CACHE_DIR/CORE-001/layout"

echo "Analyzing Copy Mode..."
analyze_c_files "Copy Mode (T-204)" "$CACHE_DIR/INTG-002"

echo "" >> "$REPORT"
echo "## Analysis Complete" >> "$REPORT"
echo "Time: $(date)" >> "$REPORT"

echo ""
echo "=== Analysis Complete ==="
echo "Report saved to: $REPORT"