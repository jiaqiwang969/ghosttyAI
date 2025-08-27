#include <dlfcn.h>
#include <stdio.h>

__attribute__((constructor))
void inject_tmux(void) {
    void *handle = dlopen("/Users/jqwang/98-ghosttyAI/tmux/libtmuxcore.dylib", RTLD_NOW | RTLD_GLOBAL);
    if (handle) {
        printf("[TMUX] libtmuxcore injected successfully\n");
    } else {
        printf("[TMUX] Failed to inject libtmuxcore: %s\n", dlerror());
    }
}
