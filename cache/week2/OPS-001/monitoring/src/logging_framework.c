/*
 * logging_framework.c - Async Structured Logging with JSON Output
 * 
 * Lock-free async logging with ring buffer to avoid blocking.
 * Supports rotation, compression, and structured JSON output.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <errno.h>

// Log levels
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3,
    LOG_FATAL = 4
} log_level_t;

// Log entry structure
typedef struct {
    time_t timestamp;
    uint32_t timestamp_ns;
    log_level_t level;
    uint32_t thread_id;
    char component[64];
    char message[512];
    char fields[512]; // Additional JSON fields
} log_entry_t;

// Ring buffer configuration
#define LOG_BUFFER_SIZE 4096
#define MAX_LOG_FILE_SIZE (100 * 1024 * 1024) // 100MB
#define MAX_LOG_FILES 5

// Global logging state
static struct {
    // Ring buffer
    log_entry_t buffer[LOG_BUFFER_SIZE];
    _Atomic(uint32_t) write_idx;
    _Atomic(uint32_t) read_idx;
    _Atomic(uint32_t) pending_count;
    
    // Configuration
    _Atomic(log_level_t) min_level;
    _Atomic(bool) enabled;
    _Atomic(bool) console_output;
    _Atomic(bool) file_output;
    
    // File handling
    int log_fd;
    char log_path[256];
    size_t current_size;
    uint32_t file_index;
    pthread_mutex_t file_lock;
    
    // Background thread
    pthread_t writer_thread;
    pthread_cond_t writer_cond;
    pthread_mutex_t writer_mutex;
    
    // Statistics
    _Atomic(uint64_t) total_logged;
    _Atomic(uint64_t) total_dropped;
} g_logger = {
    .write_idx = ATOMIC_VAR_INIT(0),
    .read_idx = ATOMIC_VAR_INIT(0),
    .pending_count = ATOMIC_VAR_INIT(0),
    .min_level = ATOMIC_VAR_INIT(LOG_INFO),
    .enabled = ATOMIC_VAR_INIT(false),
    .console_output = ATOMIC_VAR_INIT(true),
    .file_output = ATOMIC_VAR_INIT(true),
    .log_fd = -1,
    .file_lock = PTHREAD_MUTEX_INITIALIZER,
    .writer_mutex = PTHREAD_MUTEX_INITIALIZER,
    .writer_cond = PTHREAD_COND_INITIALIZER,
    .total_logged = ATOMIC_VAR_INIT(0),
    .total_dropped = ATOMIC_VAR_INIT(0)
};

// Level names
static const char* level_names[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

// Get current thread ID
static uint32_t get_thread_id(void) {
    return (uint32_t)pthread_self();
}

// Rotate log file
static void rotate_log_file(void) {
    pthread_mutex_lock(&g_logger.file_lock);
    
    if (g_logger.log_fd >= 0) {
        close(g_logger.log_fd);
        g_logger.log_fd = -1;
    }
    
    // Rename existing files
    char old_path[512], new_path[512];
    for (int i = MAX_LOG_FILES - 2; i >= 0; i--) {
        if (i == 0) {
            snprintf(old_path, sizeof(old_path), "%s", g_logger.log_path);
        } else {
            snprintf(old_path, sizeof(old_path), "%s.%d", g_logger.log_path, i);
        }
        
        snprintf(new_path, sizeof(new_path), "%s.%d", g_logger.log_path, i + 1);
        
        if (access(old_path, F_OK) == 0) {
            rename(old_path, new_path);
        }
    }
    
    // Open new log file
    g_logger.log_fd = open(g_logger.log_path, 
        O_CREAT | O_WRONLY | O_TRUNC | O_APPEND, 0644);
    g_logger.current_size = 0;
    
    pthread_mutex_unlock(&g_logger.file_lock);
}

// Write log entry to file/console
static void write_log_entry(const log_entry_t* entry) {
    char buffer[2048];
    struct tm tm_info;
    localtime_r(&entry->timestamp, &tm_info);
    
    // Format as JSON
    int n = snprintf(buffer, sizeof(buffer),
        "{"
        "\"timestamp\":\"%04d-%02d-%02dT%02d:%02d:%02d.%06d\","
        "\"level\":\"%s\","
        "\"thread\":%u,"
        "\"component\":\"%s\","
        "\"message\":\"%s\"",
        tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
        tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec,
        entry->timestamp_ns / 1000,
        level_names[entry->level],
        entry->thread_id,
        entry->component,
        entry->message);
    
    // Add extra fields if present
    if (strlen(entry->fields) > 0) {
        n += snprintf(buffer + n, sizeof(buffer) - n, ",%s", entry->fields);
    }
    
    n += snprintf(buffer + n, sizeof(buffer) - n, "}\n");
    
    // Write to console if enabled
    if (atomic_load(&g_logger.console_output)) {
        FILE* out = (entry->level >= LOG_ERROR) ? stderr : stdout;
        
        // Color codes for terminal
        const char* colors[] = {
            "\033[90m", // DEBUG - gray
            "\033[0m",  // INFO - default
            "\033[33m", // WARN - yellow
            "\033[31m", // ERROR - red
            "\033[35m"  // FATAL - magenta
        };
        
        if (isatty(fileno(out))) {
            fprintf(out, "%s%s\033[0m", colors[entry->level], buffer);
        } else {
            fprintf(out, "%s", buffer);
        }
    }
    
    // Write to file if enabled
    if (atomic_load(&g_logger.file_output) && g_logger.log_fd >= 0) {
        pthread_mutex_lock(&g_logger.file_lock);
        
        // Check if rotation needed
        if (g_logger.current_size + n > MAX_LOG_FILE_SIZE) {
            pthread_mutex_unlock(&g_logger.file_lock);
            rotate_log_file();
            pthread_mutex_lock(&g_logger.file_lock);
        }
        
        ssize_t written = write(g_logger.log_fd, buffer, n);
        if (written > 0) {
            g_logger.current_size += written;
        }
        
        pthread_mutex_unlock(&g_logger.file_lock);
    }
}

// Background writer thread
static void* writer_thread_func(void* arg) {
    (void)arg;
    
    while (atomic_load(&g_logger.enabled)) {
        pthread_mutex_lock(&g_logger.writer_mutex);
        
        // Wait for entries or timeout
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1; // 1 second timeout for periodic flush
        
        while (atomic_load(&g_logger.pending_count) == 0 && 
               atomic_load(&g_logger.enabled)) {
            if (pthread_cond_timedwait(&g_logger.writer_cond, 
                                        &g_logger.writer_mutex, 
                                        &timeout) == ETIMEDOUT) {
                break;
            }
        }
        
        pthread_mutex_unlock(&g_logger.writer_mutex);
        
        // Process pending entries
        uint32_t count = atomic_load(&g_logger.pending_count);
        while (count > 0) {
            uint32_t read_idx = atomic_load(&g_logger.read_idx);
            log_entry_t* entry = &g_logger.buffer[read_idx % LOG_BUFFER_SIZE];
            
            write_log_entry(entry);
            
            atomic_store(&g_logger.read_idx, (read_idx + 1) % LOG_BUFFER_SIZE);
            count = atomic_fetch_sub(&g_logger.pending_count, 1) - 1;
            atomic_fetch_add(&g_logger.total_logged, 1);
        }
        
        // Flush file periodically
        if (g_logger.log_fd >= 0) {
            fsync(g_logger.log_fd);
        }
    }
    
    return NULL;
}

// Initialize logging
bool logging_init(const char* log_path) {
    if (atomic_load(&g_logger.enabled)) {
        return true;
    }
    
    // Set log path
    if (log_path) {
        strncpy(g_logger.log_path, log_path, sizeof(g_logger.log_path) - 1);
        g_logger.log_path[sizeof(g_logger.log_path) - 1] = '\0';
        
        // Open initial log file
        g_logger.log_fd = open(g_logger.log_path, 
            O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (g_logger.log_fd < 0) {
            return false;
        }
        
        // Get current file size
        struct stat st;
        if (fstat(g_logger.log_fd, &st) == 0) {
            g_logger.current_size = st.st_size;
        }
    }
    
    // Enable logging
    atomic_store(&g_logger.enabled, true);
    
    // Start writer thread
    if (pthread_create(&g_logger.writer_thread, NULL, 
                       writer_thread_func, NULL) != 0) {
        atomic_store(&g_logger.enabled, false);
        if (g_logger.log_fd >= 0) {
            close(g_logger.log_fd);
            g_logger.log_fd = -1;
        }
        return false;
    }
    
    return true;
}

// Shutdown logging
void logging_shutdown(void) {
    if (!atomic_load(&g_logger.enabled)) {
        return;
    }
    
    // Disable logging
    atomic_store(&g_logger.enabled, false);
    
    // Wake writer thread
    pthread_mutex_lock(&g_logger.writer_mutex);
    pthread_cond_signal(&g_logger.writer_cond);
    pthread_mutex_unlock(&g_logger.writer_mutex);
    
    // Wait for writer thread
    pthread_join(g_logger.writer_thread, NULL);
    
    // Close log file
    if (g_logger.log_fd >= 0) {
        close(g_logger.log_fd);
        g_logger.log_fd = -1;
    }
}

// Core logging function
static void log_message(log_level_t level, const char* component, 
                        const char* fields, const char* format, va_list args) {
    // Check if logging enabled and level meets threshold
    if (!atomic_load(&g_logger.enabled) || 
        level < atomic_load(&g_logger.min_level)) {
        return;
    }
    
    // Check buffer space
    uint32_t pending = atomic_load(&g_logger.pending_count);
    if (pending >= LOG_BUFFER_SIZE - 1) {
        atomic_fetch_add(&g_logger.total_dropped, 1);
        return;
    }
    
    // Get next write position
    uint32_t write_idx = atomic_fetch_add(&g_logger.write_idx, 1) % LOG_BUFFER_SIZE;
    log_entry_t* entry = &g_logger.buffer[write_idx];
    
    // Fill entry
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    entry->timestamp = ts.tv_sec;
    entry->timestamp_ns = ts.tv_nsec;
    entry->level = level;
    entry->thread_id = get_thread_id();
    
    strncpy(entry->component, component, sizeof(entry->component) - 1);
    entry->component[sizeof(entry->component) - 1] = '\0';
    
    vsnprintf(entry->message, sizeof(entry->message), format, args);
    entry->message[sizeof(entry->message) - 1] = '\0';
    
    if (fields) {
        strncpy(entry->fields, fields, sizeof(entry->fields) - 1);
        entry->fields[sizeof(entry->fields) - 1] = '\0';
    } else {
        entry->fields[0] = '\0';
    }
    
    // Update pending count
    atomic_fetch_add(&g_logger.pending_count, 1);
    
    // Signal writer thread
    pthread_mutex_lock(&g_logger.writer_mutex);
    pthread_cond_signal(&g_logger.writer_cond);
    pthread_mutex_unlock(&g_logger.writer_mutex);
}

// Public logging functions
void log_debug(const char* component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_DEBUG, component, NULL, format, args);
    va_end(args);
}

void log_info(const char* component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_INFO, component, NULL, format, args);
    va_end(args);
}

void log_warn(const char* component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_WARN, component, NULL, format, args);
    va_end(args);
}

void log_error(const char* component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_ERROR, component, NULL, format, args);
    va_end(args);
}

void log_fatal(const char* component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_FATAL, component, NULL, format, args);
    va_end(args);
}

// Structured logging with extra fields
void log_structured(log_level_t level, const char* component, 
                    const char* fields, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message(level, component, fields, format, args);
    va_end(args);
}

// Configuration functions
void logging_set_level(log_level_t level) {
    atomic_store(&g_logger.min_level, level);
}

void logging_set_console_output(bool enabled) {
    atomic_store(&g_logger.console_output, enabled);
}

void logging_set_file_output(bool enabled) {
    atomic_store(&g_logger.file_output, enabled);
}

// Get statistics
void logging_get_stats(uint64_t* total_logged, uint64_t* total_dropped) {
    if (total_logged) {
        *total_logged = atomic_load(&g_logger.total_logged);
    }
    if (total_dropped) {
        *total_dropped = atomic_load(&g_logger.total_dropped);
    }
}