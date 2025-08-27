#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>

// tmux函数指针
int (*tmc_init)(void);
int (*tmc_input_process_key)(char key);
void (*tmc_cleanup)(void);

int main() {
    // 加载libtmuxcore
    void *handle = dlopen("./libtmuxcore.dylib", RTLD_LAZY);
    if (!handle) {
        printf("无法加载libtmuxcore: %s\n", dlerror());
        return 1;
    }
    
    // 获取函数
    tmc_init = dlsym(handle, "tmc_init");
    tmc_input_process_key = dlsym(handle, "tmc_input_process_key");
    tmc_cleanup = dlsym(handle, "tmc_cleanup");
    
    if (!tmc_init || !tmc_input_process_key || !tmc_cleanup) {
        printf("无法找到tmux函数\n");
        dlclose(handle);
        return 1;
    }
    
    // 初始化tmux
    if (tmc_init() != 0) {
        printf("tmux初始化失败\n");
        dlclose(handle);
        return 1;
    }
    
    printf("tmux初始化成功！\n");
    printf("\n模拟按键序列:\n");
    
    // 模拟Ctrl-B然后%（垂直分屏）
    printf("1. 按Ctrl-B (前缀键)...\n");
    tmc_input_process_key(0x02); // Ctrl-B
    
    sleep(1);
    
    printf("2. 按 %% (垂直分屏)...\n");
    tmc_input_process_key('%');
    
    printf("\n✓ 如果分屏成功，UI回调应该被触发\n");
    
    // 清理
    tmc_cleanup();
    dlclose(handle);
    
    return 0;
}