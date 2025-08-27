/* libtmuxcore_pty.c - PTY management for tmux panes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <util.h>
#include <signal.h>
#include <sys/wait.h>
#include "libtmuxcore_api.h"

/* PTY information structure */
typedef struct tmc_pty {
    int master_fd;
    int slave_fd;
    pid_t child_pid;
    char *slave_name;
    struct termios original_termios;
    void *pane;  /* Associated pane */
    struct tmc_pty *next;
} tmc_pty_t;

/* Global PTY list */
static tmc_pty_t *g_pty_list = NULL;

/* Create a new PTY for a pane */
tmc_error_t tmc_pty_create(tmc_pane_t pane, tmc_pty_t **pty_out) {
    tmc_pty_t *pty = calloc(1, sizeof(tmc_pty_t));
    if (!pty) {
        return TMC_ERROR_OUT_OF_MEMORY;
    }
    
    /* Open PTY master/slave pair */
    char slave_name[256];
    if (openpty(&pty->master_fd, &pty->slave_fd, slave_name, NULL, NULL) < 0) {
        free(pty);
        return TMC_ERROR_INVALID_PARAM;
    }
    
    pty->slave_name = strdup(slave_name);
    pty->pane = pane;
    
    /* Set non-blocking on master */
    int flags = fcntl(pty->master_fd, F_GETFL, 0);
    fcntl(pty->master_fd, F_SETFL, flags | O_NONBLOCK);
    
    /* Save terminal settings */
    tcgetattr(pty->slave_fd, &pty->original_termios);
    
    /* Add to global list */
    pty->next = g_pty_list;
    g_pty_list = pty;
    
    printf("[PTY] Created PTY for pane: master=%d, slave=%d, name=%s\n",
           pty->master_fd, pty->slave_fd, slave_name);
    
    *pty_out = pty;
    return TMC_SUCCESS;
}

/* Start a shell process in the PTY */
tmc_error_t tmc_pty_spawn_shell(tmc_pty_t *pty, const char *shell) {
    if (!pty) {
        return TMC_ERROR_INVALID_PARAM;
    }
    
    if (!shell) {
        shell = getenv("SHELL");
        if (!shell) {
            shell = "/bin/sh";
        }
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        return TMC_ERROR_INVALID_PARAM;
    }
    
    if (pid == 0) {
        /* Child process */
        
        /* Create new session and process group */
        setsid();
        
        /* Set controlling terminal */
        ioctl(pty->slave_fd, TIOCSCTTY, 0);
        
        /* Close master in child */
        close(pty->master_fd);
        
        /* Duplicate slave to stdin/stdout/stderr */
        dup2(pty->slave_fd, STDIN_FILENO);
        dup2(pty->slave_fd, STDOUT_FILENO);
        dup2(pty->slave_fd, STDERR_FILENO);
        
        /* Close original slave fd if it's not stdin/stdout/stderr */
        if (pty->slave_fd > STDERR_FILENO) {
            close(pty->slave_fd);
        }
        
        /* Set terminal to raw mode */
        struct termios raw = pty->original_termios;
        cfmakeraw(&raw);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        
        /* Set environment variables */
        setenv("TERM", "xterm-256color", 1);
        setenv("TMUX", "1", 1);
        
        /* Execute shell */
        execl(shell, shell, NULL);
        
        /* If exec fails */
        perror("exec");
        _exit(1);
    }
    
    /* Parent process */
    pty->child_pid = pid;
    
    /* Close slave in parent */
    close(pty->slave_fd);
    pty->slave_fd = -1;
    
    printf("[PTY] Spawned shell %s (pid=%d) on PTY %d\n", shell, pid, pty->master_fd);
    
    return TMC_SUCCESS;
}

/* Read data from PTY */
ssize_t tmc_pty_read(tmc_pty_t *pty, char *buffer, size_t size) {
    if (!pty || !buffer || size == 0) {
        return -1;
    }
    
    ssize_t n = read(pty->master_fd, buffer, size);
    if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        printf("[PTY] Read error: %s\n", strerror(errno));
    }
    
    return n;
}

/* Write data to PTY */
ssize_t tmc_pty_write(tmc_pty_t *pty, const char *data, size_t size) {
    if (!pty || !data || size == 0) {
        return -1;
    }
    
    ssize_t n = write(pty->master_fd, data, size);
    if (n < 0) {
        printf("[PTY] Write error: %s\n", strerror(errno));
    }
    
    return n;
}

/* Resize PTY window */
tmc_error_t tmc_pty_resize(tmc_pty_t *pty, int cols, int rows) {
    if (!pty) {
        return TMC_ERROR_INVALID_PARAM;
    }
    
    struct winsize ws = {
        .ws_row = rows,
        .ws_col = cols,
        .ws_xpixel = 0,
        .ws_ypixel = 0
    };
    
    if (ioctl(pty->master_fd, TIOCSWINSZ, &ws) < 0) {
        printf("[PTY] Failed to resize: %s\n", strerror(errno));
        return TMC_ERROR_INVALID_PARAM;
    }
    
    printf("[PTY] Resized to %dx%d\n", cols, rows);
    
    /* Send SIGWINCH to child process */
    if (pty->child_pid > 0) {
        kill(pty->child_pid, SIGWINCH);
    }
    
    return TMC_SUCCESS;
}

/* Check if child process is still running */
int tmc_pty_is_alive(tmc_pty_t *pty) {
    if (!pty || pty->child_pid <= 0) {
        return 0;
    }
    
    int status;
    pid_t result = waitpid(pty->child_pid, &status, WNOHANG);
    
    if (result == 0) {
        /* Child still running */
        return 1;
    } else if (result == pty->child_pid) {
        /* Child has exited */
        printf("[PTY] Child process %d exited with status %d\n", 
               pty->child_pid, WEXITSTATUS(status));
        pty->child_pid = -1;
        return 0;
    }
    
    return 0;
}

/* Close and destroy PTY */
void tmc_pty_destroy(tmc_pty_t *pty) {
    if (!pty) {
        return;
    }
    
    /* Kill child process if still running */
    if (pty->child_pid > 0) {
        kill(pty->child_pid, SIGTERM);
        usleep(100000);  /* Give it 100ms to exit */
        
        if (tmc_pty_is_alive(pty)) {
            kill(pty->child_pid, SIGKILL);
        }
        
        waitpid(pty->child_pid, NULL, 0);
    }
    
    /* Close file descriptors */
    if (pty->master_fd >= 0) {
        close(pty->master_fd);
    }
    if (pty->slave_fd >= 0) {
        close(pty->slave_fd);
    }
    
    /* Remove from global list */
    tmc_pty_t **pp = &g_pty_list;
    while (*pp) {
        if (*pp == pty) {
            *pp = pty->next;
            break;
        }
        pp = &(*pp)->next;
    }
    
    free(pty->slave_name);
    free(pty);
    
    printf("[PTY] Destroyed PTY\n");
}

/* Get PTY for a pane */
tmc_pty_t *tmc_pty_for_pane(tmc_pane_t pane) {
    tmc_pty_t *pty = g_pty_list;
    while (pty) {
        if (pty->pane == pane) {
            return pty;
        }
        pty = pty->next;
    }
    return NULL;
}

/* Process all PTY data (for event loop integration) */
void tmc_pty_process_all(void) {
    tmc_pty_t *pty = g_pty_list;
    char buffer[4096];
    
    while (pty) {
        tmc_pty_t *next = pty->next;
        
        /* Check if child is still alive */
        if (!tmc_pty_is_alive(pty)) {
            /* Child has exited, clean up */
            printf("[PTY] Child process exited for PTY %d\n", pty->master_fd);
            /* Note: In production, we'd notify the UI here */
        } else {
            /* Try to read data */
            ssize_t n = tmc_pty_read(pty, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                /* In production, we'd process this output through the grid system */
                printf("[PTY Output] %.*s", (int)n, buffer);
            }
        }
        
        pty = next;
    }
}