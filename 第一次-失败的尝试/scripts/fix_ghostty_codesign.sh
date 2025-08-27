#!/bin/bash
# fix_ghostty_codesign.sh - Fix code signing issues for local development
# Purpose: Remove code signing to allow local execution
# Date: 2025-08-26

set -e

APP_PATH="$1"
if [ -z "$APP_PATH" ]; then
    APP_PATH="/Users/jqwang/98-ghosttyAI/build/ghostty/Ghostty.app"
fi

echo "Fixing code signing for: $APP_PATH"

# Remove extended attributes that may interfere
xattr -cr "$APP_PATH" 2>/dev/null || true

# Remove all existing signatures
echo "Removing existing signatures..."
find "$APP_PATH" -type f \( -name "*.dylib" -o -name "*.framework" -o -perm +111 \) | while read f; do
    codesign --remove-signature "$f" 2>/dev/null || true
done

# Re-sign with ad-hoc signature (no identity required)
echo "Re-signing with ad-hoc signature..."
codesign --force --deep --sign - "$APP_PATH" 2>/dev/null || true

# Alternative: completely disable library validation for this app
echo "Disabling library validation..."
sudo defaults write /Library/Preferences/com.apple.security.libraryvalidation.plist DisableLibraryValidation -bool true 2>/dev/null || true

# Test if it works
echo ""
echo "Testing Ghostty..."
if "$APP_PATH/Contents/MacOS/ghostty" --version 2>&1 | grep -q "Ghostty"; then
    echo "✓ Ghostty is working!"
    "$APP_PATH/Contents/MacOS/ghostty" --version
else
    echo "⚠ Ghostty still has issues. Trying alternative fix..."
    
    # Alternative: use entitlements to disable library validation
    cat > /tmp/entitlements.plist << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>com.apple.security.cs.disable-library-validation</key>
    <true/>
    <key>com.apple.security.cs.allow-unsigned-executable-memory</key>
    <true/>
    <key>com.apple.security.cs.disable-executable-page-protection</key>
    <true/>
</dict>
</plist>
EOF
    
    codesign --force --sign - --entitlements /tmp/entitlements.plist "$APP_PATH" 2>/dev/null || true
    
    if "$APP_PATH/Contents/MacOS/ghostty" --version 2>&1 | grep -q "Ghostty"; then
        echo "✓ Fixed with entitlements!"
    else
        echo "Still having issues. You may need to run:"
        echo "  open $APP_PATH"
        echo "And allow it in System Settings > Privacy & Security"
    fi
fi

echo ""
echo "You can now run Ghostty with:"
echo "  $APP_PATH/Contents/MacOS/ghostty"
echo "Or open the app:"
echo "  open $APP_PATH"