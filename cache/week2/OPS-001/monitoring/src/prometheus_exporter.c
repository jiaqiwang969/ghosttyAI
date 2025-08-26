/*
 * prometheus_exporter.c - HTTP Server for Prometheus Metrics Export
 * 
 * Lightweight HTTP server on port 9090 for /metrics endpoint.
 * Zero-copy response generation for efficiency.
 */

#include "../include/monitoring_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define EXPORTER_PORT 9090
#define BUFFER_SIZE 65536
#define BACKLOG 10

// Global exporter state
static struct {
    int server_fd;
    pthread_t server_thread;
    volatile bool running;
    char response_buffer[BUFFER_SIZE];
    pthread_mutex_t buffer_lock;
} g_exporter = {
    .server_fd = -1,
    .running = false,
    .buffer_lock = PTHREAD_MUTEX_INITIALIZER
};

// HTTP response headers
static const char* HTTP_200_HEADER = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain; version=0.0.4\r\n"
    "Connection: close\r\n"
    "Content-Length: %zu\r\n"
    "\r\n";

static const char* HTTP_404_RESPONSE = 
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 9\r\n"
    "\r\n"
    "Not Found";

// Generate metrics response
static size_t generate_metrics_response(char* buffer, size_t max_size) {
    size_t offset = 0;
    
    // Add header comments
    offset += snprintf(buffer + offset, max_size - offset,
        "# HELP tmux_ghostty_info Build and runtime information\n"
        "# TYPE tmux_ghostty_info gauge\n"
        "tmux_ghostty_info{version=\"1.0.0\",arch=\"x86_64\"} 1\n\n");
    
    // Export all registered metrics
    int pipe_fd[2];
    if (pipe(pipe_fd) == 0) {
        metrics_export_prometheus(pipe_fd[1]);
        close(pipe_fd[1]);
        
        // Read from pipe
        ssize_t n;
        while ((n = read(pipe_fd[0], buffer + offset, max_size - offset - 1)) > 0) {
            offset += n;
        }
        close(pipe_fd[0]);
    }
    
    // Add process metrics
    FILE* status = fopen("/proc/self/status", "r");
    if (status) {
        char line[256];
        long rss = 0, threads = 0;
        
        while (fgets(line, sizeof(line), status)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line, "VmRSS: %ld", &rss);
            } else if (strncmp(line, "Threads:", 8) == 0) {
                sscanf(line, "Threads: %ld", &threads);
            }
        }
        fclose(status);
        
        offset += snprintf(buffer + offset, max_size - offset,
            "\n# HELP process_resident_memory_bytes Resident memory size in bytes\n"
            "# TYPE process_resident_memory_bytes gauge\n"
            "process_resident_memory_bytes %ld\n"
            "\n# HELP process_threads Number of OS threads\n"
            "# TYPE process_threads gauge\n"
            "process_threads %ld\n",
            rss * 1024, threads);
    }
    
    // Add CPU metrics
    FILE* stat = fopen("/proc/self/stat", "r");
    if (stat) {
        long utime, stime;
        fscanf(stat, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %ld %ld",
               &utime, &stime);
        fclose(stat);
        
        long hz = sysconf(_SC_CLK_TCK);
        double cpu_seconds = (double)(utime + stime) / hz;
        
        offset += snprintf(buffer + offset, max_size - offset,
            "\n# HELP process_cpu_seconds_total Total user and system CPU time spent in seconds\n"
            "# TYPE process_cpu_seconds_total counter\n"
            "process_cpu_seconds_total %.6f\n",
            cpu_seconds);
    }
    
    // Add file descriptor metrics
    FILE* fd_info = popen("ls /proc/self/fd | wc -l", "r");
    if (fd_info) {
        int fd_count;
        fscanf(fd_info, "%d", &fd_count);
        pclose(fd_info);
        
        offset += snprintf(buffer + offset, max_size - offset,
            "\n# HELP process_open_fds Number of open file descriptors\n"
            "# TYPE process_open_fds gauge\n"
            "process_open_fds %d\n",
            fd_count);
    }
    
    buffer[offset] = '\0';
    return offset;
}

// Handle HTTP client connection
static void handle_client(int client_fd) {
    char request[1024];
    ssize_t n = recv(client_fd, request, sizeof(request) - 1, 0);
    
    if (n <= 0) {
        close(client_fd);
        return;
    }
    
    request[n] = '\0';
    
    // Parse request
    if (strstr(request, "GET /metrics") != NULL) {
        // Generate metrics response
        pthread_mutex_lock(&g_exporter.buffer_lock);
        
        size_t content_len = generate_metrics_response(
            g_exporter.response_buffer, 
            sizeof(g_exporter.response_buffer));
        
        // Send HTTP headers
        char header[512];
        size_t header_len = snprintf(header, sizeof(header), 
            HTTP_200_HEADER, content_len);
        
        send(client_fd, header, header_len, MSG_NOSIGNAL);
        
        // Send metrics data
        send(client_fd, g_exporter.response_buffer, content_len, MSG_NOSIGNAL);
        
        pthread_mutex_unlock(&g_exporter.buffer_lock);
    } else {
        // Send 404 for other paths
        send(client_fd, HTTP_404_RESPONSE, strlen(HTTP_404_RESPONSE), MSG_NOSIGNAL);
    }
    
    close(client_fd);
}

// HTTP server thread
static void* server_thread_func(void* arg) {
    (void)arg;
    
    // Ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    
    while (g_exporter.running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        int client_fd = accept(g_exporter.server_fd, 
            (struct sockaddr*)&client_addr, &addr_len);
        
        if (client_fd < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            break;
        }
        
        // Set socket timeout
        struct timeval timeout = { .tv_sec = 5, .tv_usec = 0 };
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        
        // Handle request
        handle_client(client_fd);
    }
    
    return NULL;
}

// Start Prometheus exporter
bool prometheus_exporter_start(uint16_t port) {
    if (g_exporter.server_fd >= 0) {
        return true; // Already running
    }
    
    // Create server socket
    g_exporter.server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_exporter.server_fd < 0) {
        return false;
    }
    
    // Allow socket reuse
    int reuse = 1;
    if (setsockopt(g_exporter.server_fd, SOL_SOCKET, SO_REUSEADDR, 
                   &reuse, sizeof(reuse)) < 0) {
        close(g_exporter.server_fd);
        g_exporter.server_fd = -1;
        return false;
    }
    
    // Bind to port
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port ? port : EXPORTER_PORT)
    };
    
    if (bind(g_exporter.server_fd, (struct sockaddr*)&server_addr, 
             sizeof(server_addr)) < 0) {
        close(g_exporter.server_fd);
        g_exporter.server_fd = -1;
        return false;
    }
    
    // Start listening
    if (listen(g_exporter.server_fd, BACKLOG) < 0) {
        close(g_exporter.server_fd);
        g_exporter.server_fd = -1;
        return false;
    }
    
    // Make socket non-blocking
    int flags = fcntl(g_exporter.server_fd, F_GETFL, 0);
    fcntl(g_exporter.server_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Start server thread
    g_exporter.running = true;
    if (pthread_create(&g_exporter.server_thread, NULL, 
                       server_thread_func, NULL) != 0) {
        g_exporter.running = false;
        close(g_exporter.server_fd);
        g_exporter.server_fd = -1;
        return false;
    }
    
    return true;
}

// Stop Prometheus exporter
void prometheus_exporter_stop(void) {
    if (g_exporter.server_fd < 0) {
        return;
    }
    
    // Signal thread to stop
    g_exporter.running = false;
    
    // Close server socket to unblock accept()
    shutdown(g_exporter.server_fd, SHUT_RDWR);
    close(g_exporter.server_fd);
    g_exporter.server_fd = -1;
    
    // Wait for thread
    pthread_join(g_exporter.server_thread, NULL);
}

// Get exporter status
bool prometheus_exporter_is_running(void) {
    return g_exporter.running && g_exporter.server_fd >= 0;
}

// Get exporter URL
const char* prometheus_exporter_url(void) {
    static char url[256];
    
    if (prometheus_exporter_is_running()) {
        snprintf(url, sizeof(url), "http://localhost:%d/metrics", EXPORTER_PORT);
        return url;
    }
    
    return NULL;
}