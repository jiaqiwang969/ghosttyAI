#!/bin/bash
# modify_screen_write.sh - Add command IDs to all tty_write calls
# Purpose: Automate the modification of screen-write.c
# Date: 2025-08-26

FILE="/Users/jqwang/98-ghosttyAI/tmux/screen-write.c"
BACKUP="${FILE}.backup"

# Create backup if not exists
if [ ! -f "$BACKUP" ]; then
    cp "$FILE" "$BACKUP"
    echo "Created backup: $BACKUP"
else
    echo "Backup already exists: $BACKUP"
fi

# Function to add command ID before tty_write  
add_cmd_id() {
    local cmd_name=$1
    local cmd_id=$2
    local line_num=$3
    
    echo "Checking line ${line_num} for ${cmd_name}..."
    
    # Check if already modified
    if grep -B2 "tty_write(${cmd_name}" "$FILE" | grep -q "ui_cmd_id"; then
        echo "  Already modified: ${cmd_name} at line ${line_num}"
        return
    fi
    
    # Get the line with tty_write
    local tty_line=$(sed -n "${line_num}p" "$FILE")
    
    # Calculate indentation
    local indent=$(echo "$tty_line" | sed 's/[^ ].*//' | sed 's/	/	/g')
    
    # Insert the ifdef block before tty_write
    sed -i '' "${line_num}i\\
#ifdef LIBTMUXCORE_BUILD\\
${indent}ttyctx.ui_cmd_id = ${cmd_id};\\
#endif" "$FILE"
    
    echo "  Added ${cmd_id} for ${cmd_name}"
}

# Get line numbers for each tty_write call
echo "Finding tty_write calls..."
grep -n "tty_write(" "$FILE" | while IFS=: read -r line_num line_content; do
    if [[ $line_content == *"tty_cmd_syncstart"* ]]; then
        add_cmd_id "tty_cmd_syncstart" "UI_CMD_SYNCSTART" "$line_num"
    elif [[ $line_content == *"tty_cmd_cell"* ]] && [[ $line_content != *"tty_cmd_cells"* ]]; then
        add_cmd_id "tty_cmd_cell" "UI_CMD_CELL" "$line_num"
    elif [[ $line_content == *"tty_cmd_cells"* ]]; then
        add_cmd_id "tty_cmd_cells" "UI_CMD_CELLS" "$line_num"
    elif [[ $line_content == *"tty_cmd_alignmenttest"* ]]; then
        add_cmd_id "tty_cmd_alignmenttest" "UI_CMD_ALIGNMENTTEST" "$line_num"
    elif [[ $line_content == *"tty_cmd_insertcharacter"* ]]; then
        add_cmd_id "tty_cmd_insertcharacter" "UI_CMD_INSERTCHARACTER" "$line_num"
    elif [[ $line_content == *"tty_cmd_deletecharacter"* ]]; then
        add_cmd_id "tty_cmd_deletecharacter" "UI_CMD_DELETECHARACTER" "$line_num"
    elif [[ $line_content == *"tty_cmd_clearcharacter"* ]]; then
        add_cmd_id "tty_cmd_clearcharacter" "UI_CMD_CLEARCHARACTER" "$line_num"
    elif [[ $line_content == *"tty_cmd_insertline"* ]]; then
        add_cmd_id "tty_cmd_insertline" "UI_CMD_INSERTLINE" "$line_num"
    elif [[ $line_content == *"tty_cmd_deleteline"* ]]; then
        add_cmd_id "tty_cmd_deleteline" "UI_CMD_DELETELINE" "$line_num"
    elif [[ $line_content == *"tty_cmd_reverseindex"* ]]; then
        add_cmd_id "tty_cmd_reverseindex" "UI_CMD_REVERSEINDEX" "$line_num"
    elif [[ $line_content == *"tty_cmd_scrolldown"* ]]; then
        add_cmd_id "tty_cmd_scrolldown" "UI_CMD_SCROLLDOWN" "$line_num"
    elif [[ $line_content == *"tty_cmd_scrollup"* ]]; then
        add_cmd_id "tty_cmd_scrollup" "UI_CMD_SCROLLUP" "$line_num"
    elif [[ $line_content == *"tty_cmd_clearendofscreen"* ]]; then
        add_cmd_id "tty_cmd_clearendofscreen" "UI_CMD_CLEARENDOFSCREEN" "$line_num"
    elif [[ $line_content == *"tty_cmd_clearstartofscreen"* ]]; then
        add_cmd_id "tty_cmd_clearstartofscreen" "UI_CMD_CLEARSTARTOFSCREEN" "$line_num"
    elif [[ $line_content == *"tty_cmd_clearscreen"* ]]; then
        add_cmd_id "tty_cmd_clearscreen" "UI_CMD_CLEARSCREEN" "$line_num"
    elif [[ $line_content == *"tty_cmd_setselection"* ]]; then
        add_cmd_id "tty_cmd_setselection" "UI_CMD_SETSELECTION" "$line_num"
    elif [[ $line_content == *"tty_cmd_rawstring"* ]]; then
        add_cmd_id "tty_cmd_rawstring" "UI_CMD_RAWSTRING" "$line_num"
    elif [[ $line_content == *"tty_cmd_sixelimage"* ]]; then
        add_cmd_id "tty_cmd_sixelimage" "UI_CMD_SIXELIMAGE" "$line_num"
    fi
done

echo ""
echo "Modifications complete!"

# Verify changes
echo ""
echo "Verification:"
echo "Number of tty_write calls:"
grep -c "tty_write(" "$FILE"
echo "Number of ui_cmd_id assignments:"
grep -c "ui_cmd_id" "$FILE"