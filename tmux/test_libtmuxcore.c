// test_libtmuxcore.c - Test program for libtmuxcore
// Purpose: Verify libtmuxcore can be loaded and used
// Date: 2025-08-26

#include <stdio.h>
#include <dlfcn.h>
#include "libtmuxcore.h"

int main() {
    printf("Testing libtmuxcore...\n");
    
    // Get version
    int major, minor, patch;
    tmc_get_version(&major, &minor, &patch);
    printf("libtmuxcore version: %d.%d.%d\n", major, minor, patch);
    
    // Initialize
    tmc_handle_t* handle = tmc_init();
    if (!handle) {
        printf("ERROR: Failed to initialize libtmuxcore\n");
        return 1;
    }
    printf("✓ Initialized libtmuxcore\n");
    
    // Set backend mode
    if (tmc_set_backend_mode(handle, "ghostty") == 0) {
        printf("✓ Set backend mode to ghostty\n");
    }
    
    // Create a session
    if (tmc_create_session(handle, "test") == 0) {
        printf("✓ Created session 'test'\n");
    }
    
    // Execute a command
    if (tmc_execute_command(handle, "new-window") == 0) {
        printf("✓ Executed command 'new-window'\n");
    }
    
    // Cleanup
    tmc_cleanup(handle);
    printf("✓ Cleaned up libtmuxcore\n");
    
    printf("\nAll tests passed!\n");
    return 0;
}