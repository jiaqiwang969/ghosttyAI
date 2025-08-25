// backend_ghostty_simplified.c - Simplified Ghostty Backend for Testing
// Purpose: Simplified implementation for integration testing
// Date: 2025-08-25

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "ui_backend.h"
#include "backend_router.h"

// Simplified Ghostty backend structure
typedef struct {
    struct ui_backend base;  // Base interface
    int initialized;
    int call_count;
    ui_backend_ops_t ops;    // Operations table
} ghostty_backend_t;

// Get current time in nanoseconds
static uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Command implementations
static void ghostty_cmd_cell(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_cells(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_clearline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_clearscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_insertline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_deleteline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_insertcharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_deletecharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_clearcharacter(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_clearendofline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_clearstartofline(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_clearendofscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_clearstartofscreen(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_alignmenttest(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_reverseindex(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_linefeed(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_scrollup(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_scrolldown(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_setselection(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_rawstring(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_sixelimage(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

static void ghostty_cmd_syncstart(struct ui_backend* backend, const struct tty_ctx* ctx) {
    (void)ctx;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    gb->call_count++;
}

// Create Ghostty backend
struct ui_backend* ghostty_backend_create(void* ghostty_handle) {
    (void)ghostty_handle;
    
    ghostty_backend_t* backend = calloc(1, sizeof(ghostty_backend_t));
    if (!backend) return NULL;
    
    backend->initialized = 1;
    backend->call_count = 0;
    
    // Set up function pointers
    backend->base.cmd_cell = ghostty_cmd_cell;
    backend->base.cmd_cells = ghostty_cmd_cells;
    backend->base.cmd_clearline = ghostty_cmd_clearline;
    backend->base.cmd_clearscreen = ghostty_cmd_clearscreen;
    backend->base.cmd_insertline = ghostty_cmd_insertline;
    backend->base.cmd_deleteline = ghostty_cmd_deleteline;
    backend->base.cmd_insertcharacter = ghostty_cmd_insertcharacter;
    backend->base.cmd_deletecharacter = ghostty_cmd_deletecharacter;
    backend->base.cmd_clearcharacter = ghostty_cmd_clearcharacter;
    backend->base.cmd_clearendofline = ghostty_cmd_clearendofline;
    backend->base.cmd_clearstartofline = ghostty_cmd_clearstartofline;
    backend->base.cmd_clearendofscreen = ghostty_cmd_clearendofscreen;
    backend->base.cmd_clearstartofscreen = ghostty_cmd_clearstartofscreen;
    backend->base.cmd_alignmenttest = ghostty_cmd_alignmenttest;
    backend->base.cmd_reverseindex = ghostty_cmd_reverseindex;
    backend->base.cmd_linefeed = ghostty_cmd_linefeed;
    backend->base.cmd_scrollup = ghostty_cmd_scrollup;
    backend->base.cmd_scrolldown = ghostty_cmd_scrolldown;
    backend->base.cmd_setselection = ghostty_cmd_setselection;
    backend->base.cmd_rawstring = ghostty_cmd_rawstring;
    backend->base.cmd_sixelimage = ghostty_cmd_sixelimage;
    backend->base.cmd_syncstart = ghostty_cmd_syncstart;
    
    return &backend->base;
}

// Destroy Ghostty backend
void ghostty_backend_destroy(struct ui_backend* backend) {
    if (backend) {
        free(backend);
    }
}

// Get statistics
int ghostty_backend_get_call_count(struct ui_backend* backend) {
    if (!backend) return 0;
    ghostty_backend_t* gb = (ghostty_backend_t*)backend;
    return gb->call_count;
}