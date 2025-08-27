#include "libtmuxcore_api.h"
#include <stdio.h>

int main() {
    tmc_init();
    
    /* Create a demo session */
    tmc_session_t session;
    tmc_session_new("demo", &session);
    tmc_session_attach(session);
    
    /* Create windows */
    tmc_window_t w1, w2;
    tmc_window_new(session, "editor", &w1);
    tmc_window_new(session, "terminal", &w2);
    
    /* Split panes */
    tmc_pane_t p1, p2;
    tmc_pane_split(w1, 1, 50, &p1);  /* Horizontal */
    tmc_pane_split(w1, 0, 50, &p2);  /* Vertical */
    
    /* Show structure */
    printf("\n=== tmux Structure ===\n");
    tmc_command_execute("list-sessions");
    printf("\n");
    tmc_command_execute("list-windows");
    printf("\n");
    tmc_command_execute("list-panes");
    
    tmc_cleanup();
    return 0;
}
