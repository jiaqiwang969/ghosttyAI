#!/bin/bash
# tmux_wrapper.sh - Wrapper to demonstrate tmux in Ghostty
# This simulates what the deep integration would do

# Check if this is a @tmux command
if [[ "$1" == "@tmux" ]]; then
    shift  # Remove @tmux
    
    case "$1" in
        "new-session")
            echo "[Ghostty+tmux] Creating session: ${2:-main}"
            tmux new-session -s "${2:-main}"
            ;;
        "list-sessions"|"ls")
            echo "[Ghostty+tmux] Available sessions:"
            tmux list-sessions 2>/dev/null || echo "  No sessions found"
            ;;
        "attach"|"attach-session"|"a")
            echo "[Ghostty+tmux] Attaching to session: ${2:-main}"
            tmux attach-session -t "${2:-main}"
            ;;
        "detach")
            echo "[Ghostty+tmux] Detaching..."
            tmux detach-client
            ;;
        *)
            echo "[Ghostty+tmux] Available commands:"
            echo "  @tmux new-session <name>  - Create new session"
            echo "  @tmux list-sessions      - List all sessions"
            echo "  @tmux attach <name>      - Attach to session"
            echo "  @tmux detach            - Detach from session"
            ;;
    esac
else
    # Not a @tmux command, just run it normally
    exec "$@"
fi