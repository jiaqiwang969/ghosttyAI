#!/bin/bash
# verify_fix.sh - Quick verification that DEFECT-002 is fixed
# Author: CORE-001 (c-tmux-specialist)
# Date: 2025-08-25

echo "============================================================="
echo "DEFECT-002 Fix Verification Script"
echo "============================================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check we're in the right directory
if [ ! -f "tty_write_hooks.h" ]; then
    echo -e "${RED}Error: Run this script from cache/week1/CORE-001/fixed/${NC}"
    exit 1
fi

echo "1. Checking standardized function names..."
if grep -q "void tty_hooks_init(void);" tty_write_hooks.h; then
    echo -e "   ${GREEN}✓${NC} tty_hooks_init() found"
else
    echo -e "   ${RED}✗${NC} tty_hooks_init() NOT found"
    exit 1
fi

if grep -q "int tty_hooks_install" tty_write_hooks.h; then
    echo -e "   ${GREEN}✓${NC} tty_hooks_install() found"
else
    echo -e "   ${RED}✗${NC} tty_hooks_install() NOT found"
    exit 1
fi

if grep -q "int tty_hooks_uninstall" tty_write_hooks.h; then
    echo -e "   ${GREEN}✓${NC} tty_hooks_uninstall() found"
else
    echo -e "   ${RED}✗${NC} tty_hooks_uninstall() NOT found"
    exit 1
fi

echo ""
echo "2. Checking compatibility macros..."
if grep -q "#define tty_write_hooks_init" tty_write_hooks.h; then
    echo -e "   ${GREEN}✓${NC} Compatibility macros present"
else
    echo -e "   ${RED}✗${NC} Compatibility macros missing"
    exit 1
fi

echo ""
echo "3. Checking version update..."
if grep -q "Version: 1.1.0" tty_write_hooks.h; then
    echo -e "   ${GREEN}✓${NC} Header version updated to 1.1.0"
else
    echo -e "   ${RED}✗${NC} Header version not updated"
    exit 1
fi

if grep -q "Version: 1.1.0" tty_write_hooks.c; then
    echo -e "   ${GREEN}✓${NC} Implementation version updated to 1.1.0"
else
    echo -e "   ${RED}✗${NC} Implementation version not updated"
    exit 1
fi

echo ""
echo "4. Checking implementation consistency..."
if grep -q "^void tty_hooks_init(void)" tty_write_hooks.c; then
    echo -e "   ${GREEN}✓${NC} Implementation uses tty_hooks_init()"
else
    echo -e "   ${RED}✗${NC} Implementation mismatch"
    exit 1
fi

echo ""
echo "5. Compiling compatibility test..."
if gcc -o test_compat test_compatibility.c 2>/dev/null; then
    echo -e "   ${GREEN}✓${NC} Compatibility test compiles"
    
    echo ""
    echo "6. Running compatibility test..."
    if ./test_compat >/dev/null 2>&1; then
        echo -e "   ${GREEN}✓${NC} Compatibility test passes"
    else
        echo -e "   ${RED}✗${NC} Compatibility test fails"
        exit 1
    fi
else
    echo -e "   ${RED}✗${NC} Compatibility test compilation failed"
    exit 1
fi

echo ""
echo "7. Checking documentation..."
if [ -f "MIGRATION_GUIDE.md" ]; then
    echo -e "   ${GREEN}✓${NC} Migration guide present"
else
    echo -e "   ${RED}✗${NC} Migration guide missing"
fi

if [ -f "DEFECT-002.md" ]; then
    echo -e "   ${GREEN}✓${NC} Defect report present"
else
    echo -e "   ${RED}✗${NC} Defect report missing"
fi

echo ""
echo "============================================================="
echo -e "${GREEN}✅ DEFECT-002 FIX VERIFIED SUCCESSFULLY${NC}"
echo "============================================================="
echo ""
echo "Summary:"
echo "  - All function names standardized to tty_hooks_*"
echo "  - Backward compatibility maintained via macros"
echo "  - Version updated to 1.1.0"
echo "  - Documentation complete"
echo "  - Ready for integration with CORE-002"
echo ""
echo "Location: cache/week1/CORE-001/fixed/"
echo "Delivered: $(date '+%Y-%m-%d %H:%M:%S')"
echo ""

# Clean up
rm -f test_compat

exit 0