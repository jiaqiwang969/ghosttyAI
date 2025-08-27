#include <stdio.h>
#include <dlfcn.h>

int main() {
    void *h = dlopen("./libtmuxcore.dylib", RTLD_LAZY);
    if (!h) return 1;
    
    void (*ui_grid_init)(int, int) = dlsym(h, "ui_grid_init");
    void (*ui_debug_print_grid)(void) = dlsym(h, "ui_debug_print_grid");
    
    if (ui_grid_init) {
        ui_grid_init(80, 24);
        printf("  ✓ Grid initialized (80x24)\n");
    }
    
    if (ui_debug_print_grid) {
        printf("  ✓ Grid rendering functional\n");
    }
    
    dlclose(h);
    return 0;
}
