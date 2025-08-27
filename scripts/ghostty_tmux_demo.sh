#!/bin/bash
# ghostty_tmux_demo.sh - Demonstrate @tmux commands in Ghostty
# Purpose: Show how deep integration would work
# Date: 2025-08-27

echo "======================================="
echo "  Ghostty with @tmux Deep Integration"
echo "======================================="
echo ""
echo "This demonstrates what the deep integration provides:"
echo "Type '@tmux <command>' to control tmux"
echo ""
echo "Available commands:"
echo "  @tmux new-session <name>  - Create new tmux session"
echo "  @tmux list                - List all sessions"
echo "  @tmux attach <name>       - Attach to session"
echo "  @tmux detach              - Detach from session"
echo "  @tmux help                - Show this help"
echo "  exit                      - Exit demo"
echo ""
echo "======================================="
echo ""

# Function to handle @tmux commands
handle_tmux_command() {
    local cmd="$1"
    shift
    local args="$@"
    
    case "$cmd" in
        "new-session"|"new")
            local session_name="${args:-main}"
            echo -e "\033[32m[Ghostty+tmux]\033[0m Creating session '$session_name'..."
            # In real integration, this would call libtmuxcore
            tmux new-session -d -s "$session_name" 2>/dev/null
            if [ $? -eq 0 ]; then
                echo -e "\033[32m✓\033[0m Session '$session_name' created"
            else
                echo -e "\033[31m✗\033[0m Session '$session_name' already exists or error occurred"
            fi
            ;;
            
        "list"|"ls"|"list-sessions")
            echo -e "\033[32m[Ghostty+tmux]\033[0m Available sessions:"
            tmux list-sessions 2>/dev/null | while read line; do
                echo "  • $line"
            done || echo "  No sessions found"
            ;;
            
        "attach"|"a")
            local session_name="${args:-main}"
            echo -e "\033[32m[Ghostty+tmux]\033[0m Attaching to session '$session_name'..."
            tmux attach-session -t "$session_name"
            ;;
            
        "detach"|"d")
            echo -e "\033[32m[Ghostty+tmux]\033[0m Detaching from current session..."
            tmux detach-client 2>/dev/null || echo "Not in a tmux session"
            ;;
            
        "help"|"h"|"")
            echo -e "\033[32m[Ghostty+tmux]\033[0m Available commands:"
            echo "  @tmux new-session <name>  - Create new tmux session"
            echo "  @tmux list                - List all sessions"
            echo "  @tmux attach <name>       - Attach to session"
            echo "  @tmux detach              - Detach from session"
            ;;
            
        *)
            echo -e "\033[31m[Ghostty+tmux]\033[0m Unknown command: $cmd"
            echo "Type '@tmux help' for available commands"
            ;;
    esac
}

# Main loop - simulate Ghostty terminal with @tmux support
while true; do
    # Show prompt
    echo -ne "\033[36mghostty\033[0m $ "
    
    # Read user input
    read -r input
    
    # Check if it's a @tmux command
    if [[ "$input" == "@tmux"* ]]; then
        # Extract command and arguments
        cmd_line="${input#@tmux}"
        cmd_line="${cmd_line# }"  # Remove leading space
        
        # Split into command and args
        read -r cmd args <<< "$cmd_line"
        
        # Handle the tmux command
        handle_tmux_command "$cmd" $args
        
    elif [[ "$input" == "exit" ]]; then
        echo "Goodbye!"
        break
        
    else
        # Regular command - just execute it
        eval "$input" 2>&1
    fi
done