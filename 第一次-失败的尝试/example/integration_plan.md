# Ghostty × libtmuxcore 完全内嵌集成计划（详细）

> 目标：将 tmux 的 **Server/Session/Window/Pane/Grid/History/Layout** 等核心以 **libtmuxcore** 作为稳定 C ABI 输出，Ghostty 以 Zig FFI 内嵌调用；不输出终端转义，直接以结构化网格与事件回调驱动 Ghostty 渲染和输入。

---

## 一、tmux 代码分层与切割

### 需要保留（进入 libtmuxcore）
- `server.c`, `proc.c`, `job.c`：服务器与进程/PTY 管理（保持 libevent 依赖，但通过 **loop vtable** 抽象）。
- `session.c`, `window.c`, `window-tree.c`, `layout-*.c`：会话/窗口/面板与布局树。
- `grid.c`, `grid-view.c`, `history.c`, `screen.c`, `screen-redraw.c`, `screen-write.c`：屏幕网格与写入逻辑（**在 tty 写出前挂接**）。
- `options*.c`：选项系统；`format.c`（必要部分）。
- `cmd-*.c`：命令体系（保留，作为 API `tmc_command()` 的实现）。
- `mode-tree.c`、`copy-mode*.c` 等交互模式逻辑（UI 状态经回调暴露）。
- `notify.c`, `hooks.c`：事件/钩子（映射到 `on_message`/专用回调）。

### 需要替换/抽象
- `tty.c`, `tty-*.c`, `status.c`, `input-keys.c`, `xterm-keys.c`：
  - **不再输出转义序列**；改为调用 **UI backend vtable**（见「挂接点」）。
  - 状态栏/边框绘制移交给 Ghostty，core 仅给出布局树与 pane 元数据。
- `client.c`：代表“一个附件”的生命周期保留，但 I/O 改成内嵌 UI backend；新增客户端类型 `CLIENT_EMBEDDED`。

### 可剥离/禁用
- `control.c`/`control-notify.c`（控制模式）在内嵌场景可编译关闭。
- 平台专属 `osdep-*.c` 保留（PTY/进程）；若需要 Windows 适配，另行实现。

---

## 二、挂接点设计（最小侵入）

1. **替换 `tty_write()` 出口**  
   在 `screen-write.c` 各种 `tty_write(tty_cmd_*, &ttyctx)` 处，原逻辑是“把 screen 的脏区翻译成 termcap 序列”。  
   **改造**：在 `tty.c` 引入 **UI Backend VTable**，对每个 `tty_cmd_*` 提供一个等价的“结构化绘制”分支（例如 `ui->draw_cells(spans)`、`ui->cursor(x,y)`、`ui->clear(...)`）。
   - 保持原 TTY 后端（用于兼容原生终端/单元测试）。
   - 新增 `tty_backend_ghostty.c`：把命令聚合为 `tmc_grid_update`，合并同 row 的 runs，走回调。

2. **复制模式与选择**  
   在 `copy-mode*.c` 更新光标/选区时，调用 `ui->copy_mode_state(pane, cursor, selection)`。Ghostty 以原生方式绘制高亮，鼠标事件回注入 `tmc_mouse()`。

3. **布局树**  
   在 `layout-*.c` 完成/变更后，调用 `ui->layout(window_id, nodes[])`，每个 node 给出 `x/y/w/h` 与 `pane_id`。Ghostty 负责边框与分割线。

4. **事件循环**  
   用 `tmc_loop_vtable` 把 `libevent` 的 fd/timer 注册改成“委托给宿主”（Ghostty 事件循环）。保持内部仍使用 event 回调 API，但实现换成 vtable。

---

## 三、C ABI（libtmuxcore.h）

见同目录 `libtmuxcore.h`：
- 事件回调：`on_grid / on_layout / on_copy_mode / on_title / on_session / on_process_exit / on_message`
- 输入 API：`tmc_send_keys / tmc_send_text / tmc_mouse`
- 生命周期：`tmc_server_new/free`、`tmc_client_attach/detach/resize`
- 命令：`tmc_command(fmt, ...)` （与 tmux 现有 `cmd-*` 共用）
- 快照：`tmc_pane_snapshot()`（全量获取）
- 选项：`tmc_set_option / tmc_get_option`（直通 tmux 选项系统）

---

## 四、Ghostty 侧集成（Zig）

- FFI 样例见 `ghostty_tmx_bridge.zig`：
  - 将 `on_grid` 映射到 Ghostty 的 `Terminal.Buffer`：把每个 `span` 写入对应 pane 的缓冲，打脏并由 GPU 合成。
  - `on_layout` → 重建 split 树；Ghostty 绘制边框。
  - `on_copy_mode` → 切换输入路由（屏蔽文本直通，优先 copy-mode 导航）。
- 输入路由：现有 `terminal/tmux.zig`（控制模式）可作为键鼠映射参考；改为 `tmc_send_*`。

---

## 五、构建与目录建议

在 tmux 源树新增：
```
/libtmuxcore/
  ui_backend.h           // vtable 定义（与 libtmuxcore.h 对齐）
  backend_ghostty.c      // 实现结构化绘制，聚合 spans
  loop_shim.c            // 把 libevent 调用改为 vtable（或自有轻量 reactor）
  libtmuxcore.c          // 组装导出符号，桥接 cmd/option/ids 等
  export.map             // 版本脚本（ELF）
```
顶层构建：
- 生成 `libtmuxcore.so/.dylib/.a`；
- 保留 `tmux` 可执行（链接 tty 后端）。

Ghostty 侧：
- `src/mux/libtmuxcore.zig`（对 `libtmuxcore.h` 的 FFI）
- `build.zig`：按平台链接；提供 feature flag `-Dtmuxcore=true` 切换。

---

## 六、迁移步骤（建议 3~5 周）

- **W1**
  1. 提炼 `ui_backend.h`，给 `tty_write` 增加多后端路由（不破坏旧行为）。
  2. 做最小后端：把 `tty_cmd_cell` 聚合为 `grid spans`，回调打印到日志（验证调用链）。

- **W2**
  3. 完成 `layout`/`copy-mode` 回调；Ghostty 侧跑通“空白 pane + 光标移动 + 文本回显”。
  4. 事件循环抽象（fd/timer），替换内部分发为 vtable。

- **W3**
  5. 完成历史滚动/捕获、搜索（由 core 提供，UI 展示）。
  6. 键鼠路由：prefixless、SGR 鼠标、wheel/拖拽。

- **W4+**
  7. 压测与背压：合并 spans、限制回调频率（最多每 vsync 送一次）。
  8. 选项映射、`.tmux.conf` 兼容层；崩溃恢复与“会话可被其它终端接入”。

---

## 七、兼容性与许可证

- tmux 采用宽松许可证（见 `tmux.h` 头部 ISC 风格）；
- Ghostty 以自身许可为准；本方案以 **动态链接或同仓构建静态库** 两种方式均可行。

---

## 八、测试策略

- 单元测试：grid diff、宽字符、emoji、双宽、属性合并、超长行 wrap、滚动边界。
- 回归：`vim/neovim`、`htop`、`less`、`git log --graph`、sixel/kitty 图片（如启用）。
- Fuzz：核心 `input.c`/`screen-write.c` 序列与状态机。
- 性能：大吞吐（`yes | pv`）、全屏重绘（`vim :set rnu`）、多 pane 同步输出。

---

## 九、后续增强

- 二进制协议（共享内存传递 cells）、零拷贝历史快照；
- 后台 session 可被多个 Ghostty 附着（多客户端渲染同一 core）；
- 统一剪贴板寄存器：Ghostty 提供系统剪贴板，core 仅维护命名 buffer。
