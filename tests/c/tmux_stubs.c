// Minimal stubs for testing
#include "tmux/ui_backend/ui_backend_minimal.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// UTF8 data stub
void utf8_set(struct utf8_data* ud, u_char ch) {
    ud->size = 1;
    ud->data[0] = ch;
    memset(ud->data + 1, 0, sizeof(ud->data) - 1);
}

// Grid functions stubs
void* grid_create(u_int sx, u_int sy, u_int hlimit) {
    (void)sx; (void)sy; (void)hlimit;
    return NULL;
}

void grid_destroy(void* g) {
    (void)g;
}

// Event stubs (minimal)
void* event_init(void) {
    return NULL;
}

int event_base_loop(void* base, int flags) {
    (void)base; (void)flags;
    return 0;
}

void event_base_free(void* base) {
    (void)base;
}

// TTY command function stubs for testing
void tty_cmd_cell(struct tty* tty, const struct tty_ctx* ctx) {
    (void)tty; (void)ctx;
    printf("[STUB] tty_cmd_cell called\n");
}

void tty_cmd_clearline(struct tty* tty, const struct tty_ctx* ctx) {
    (void)tty; (void)ctx;
    printf("[STUB] tty_cmd_clearline called\n");
}

void tty_cmd_clearscreen(struct tty* tty, const struct tty_ctx* ctx) {
    (void)tty; (void)ctx;
    printf("[STUB] tty_cmd_clearscreen called\n");
}

void tty_cmd_insertline(struct tty* tty, const struct tty_ctx* ctx) {
    (void)tty; (void)ctx;
    printf("[STUB] tty_cmd_insertline called\n");
}

void tty_cmd_deleteline(struct tty* tty, const struct tty_ctx* ctx) {
    (void)tty; (void)ctx;
    printf("[STUB] tty_cmd_deleteline called\n");
}

void tty_cmd_clearendofline(struct tty* tty, const struct tty_ctx* ctx) {
    (void)tty; (void)ctx;
    printf("[STUB] tty_cmd_clearendofline called\n");
}

void tty_cmd_reverseindex(struct tty* tty, const struct tty_ctx* ctx) {
    (void)tty; (void)ctx;
    printf("[STUB] tty_cmd_reverseindex called\n");
}

void tty_cmd_linefeed(struct tty* tty, const struct tty_ctx* ctx) {
    (void)tty; (void)ctx;
    printf("[STUB] tty_cmd_linefeed called\n");
}

void tty_cmd_scrollup(struct tty* tty, const struct tty_ctx* ctx) {
    (void)tty; (void)ctx;
    printf("[STUB] tty_cmd_scrollup called\n");
}

void tty_cmd_scrolldown(struct tty* tty, const struct tty_ctx* ctx) {
    (void)tty; (void)ctx;
    printf("[STUB] tty_cmd_scrolldown called\n");
}
