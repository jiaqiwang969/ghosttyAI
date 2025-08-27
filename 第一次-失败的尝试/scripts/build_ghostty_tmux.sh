#!/bin/bash
# build_ghostty_tmux.sh - Build script for Ghostty tmux integration
# Purpose: Build and test Week 5 Ghostty integration
# Date: 2025-08-26
# Task: T-502 - Connect Terminal.zig to callbacks

set -e

echo "====================================="
echo "  Building Ghostty tmux Integration"
echo "====================================="
echo ""

# Navigate to project root
cd /Users/jqwang/98-ghosttyAI

# Step 1: Ensure libtmuxcore is built with command IDs
echo "Step 1: Building libtmuxcore..."
if [ ! -f libtmuxcore.dylib ]; then
    echo "Building libtmuxcore.dylib..."
    gcc -DLIBTMUXCORE_BUILD -dynamiclib -o libtmuxcore.dylib \
        tmux/ui_backend/ui_backend.c \
        tmux/ui_backend/ui_backend_dispatch.c \
        tmux/ui_backend/event_loop_router_minimal.c \
        tmux/ui_backend/event_loop_router_stub.c \
        tmux_stubs.c \
        -I. -Itmux -Itmux/ui_backend
    echo "✓ libtmuxcore.dylib created"
else
    echo "✓ libtmuxcore.dylib already exists"
fi

# Step 2: Build Ghostty FFI bridge test
echo ""
echo "Step 2: Building Ghostty FFI bridge test..."

cd ghostty/src/tmux

# Create a simple build.zig for our test
cat > build_test.zig << 'EOF'
const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    
    // Test executable
    const test_exe = b.addExecutable(.{
        .name = "test_ghostty_tmux",
        .root_source_file = b.path("test_ghostty_tmux.zig"),
        .target = target,
        .optimize = optimize,
    });
    
    // Link with libtmuxcore
    test_exe.linkSystemLibrary("tmuxcore");
    test_exe.addLibraryPath(b.path("../../.."));
    test_exe.addIncludePath(b.path("../../.."));
    
    // Add Ghostty modules path
    test_exe.addIncludePath(b.path(".."));
    
    b.installArtifact(test_exe);
    
    // Run command
    const run_cmd = b.addRunArtifact(test_exe);
    run_cmd.step.dependOn(b.getInstallStep());
    
    const run_step = b.step("run", "Run the test");
    run_step.dependOn(&run_cmd.step);
}
EOF

# Try to build with zig
if command -v zig &> /dev/null; then
    echo "Building with Zig..."
    zig build -Doptimize=Debug --build-file build_test.zig
    
    if [ $? -eq 0 ]; then
        echo "✓ Ghostty test built successfully"
    else
        echo "⚠ Zig build failed, trying simplified approach..."
    fi
else
    echo "⚠ Zig not found, skipping Ghostty build"
fi

cd ../../..

# Step 3: Run our existing minimal test to verify tmux side works
echo ""
echo "Step 3: Testing tmux dispatch with command IDs..."
if [ -f test_minimal ]; then
    ./test_minimal | tail -10
else
    echo "test_minimal not found, building..."
    gcc -o test_minimal test_minimal_dispatch.c \
        -L. -ltmuxcore -Wl,-rpath,. -DLIBTMUXCORE_BUILD
    ./test_minimal | tail -10
fi

echo ""
echo "====================================="
echo "       Build Summary"
echo "====================================="

# Check what was built
echo ""
echo "Built artifacts:"
ls -lh libtmuxcore.dylib 2>/dev/null && echo "✓ libtmuxcore.dylib"
ls -lh test_minimal 2>/dev/null && echo "✓ test_minimal"
ls -lh ghostty/src/tmux/zig-out/bin/test_ghostty_tmux 2>/dev/null && echo "✓ test_ghostty_tmux"

echo ""
echo "Next steps:"
echo "1. Modify Termio.zig to add tmux initialization"
echo "2. Add config option for enable_tmux"
echo "3. Run full Ghostty with tmux support"
echo ""
echo "To test integration manually:"
echo "  export DYLD_LIBRARY_PATH=."
echo "  ./ghostty/src/tmux/zig-out/bin/test_ghostty_tmux"
echo ""
echo "====================================="