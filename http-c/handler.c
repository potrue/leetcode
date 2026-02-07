#include "handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INITIAL_READ_BUFFER_SIZE 1024

static void respond_and_close(int connection_fd, int status_code, const char* body) {
    char* status_message = NULL;
    switch (status_code) {
        case 200: status_message = "OK";                    break;
        case 400: status_message = "Bad Request";           break;
        case 404: status_message = "Not Found";             break;
        case 500: status_message = "Internal Server Error"; break;
        default:  status_message = "Unknown Status";        break;
    }

    char* response = NULL;
    int response_len = asprintf(&response,
        "HTTP/1.1 %d %s\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s",
        status_code, status_message, strlen(body), body
    );

    if (response_len == -1) {
        perror("asprintf");
        close(connection_fd);
        return;
    }

    write(connection_fd, response, response_len);
    free(response);
    close(connection_fd);
}

static char* read_request(int connection_fd) {
    size_t capacity = INITIAL_READ_BUFFER_SIZE;
    char* read_buffer = malloc(capacity);
    size_t total_read_size = 0;

    while (1) {
        ssize_t read_size = read(connection_fd, read_buffer + total_read_size, capacity - total_read_size - 1);
        if (read_size < 0) {
            perror("read");
            free(read_buffer);
            return NULL;
        }
        if (read_size == 0) {
            break;
        }

        // "\r\n\r\n" (4文字) が読み込み境界を跨ぐ可能性を考慮し、3バイト戻ったところから探索する
        size_t str_search_offset = total_read_size > 3 ? total_read_size - 3 : 0;

        total_read_size += read_size;
        read_buffer[total_read_size] = '\0';

        if (strstr(read_buffer + str_search_offset, "\r\n\r\n")) {
            break;
        } 

        if (total_read_size == capacity - 1) {
            capacity *= 2;
            char* new_read_buffer = realloc(read_buffer, capacity);
            if (new_read_buffer == NULL) {
                perror("realloc");
                free(read_buffer);
                return NULL;
            }
            read_buffer = new_read_buffer;
        }
    }

    return read_buffer;
}

void handle_connection(int connection_fd) {
    const char* path_prefix = "GET /calc";
    const char* query_prefix = "?query=";

    char* read_buffer = read_request(connection_fd);
    if (read_buffer == NULL) {
        respond_and_close(connection_fd, 500, "Something went wrong\n");
        return;
    }

    if (strncmp(read_buffer, path_prefix, strlen(path_prefix)) != 0) {
        respond_and_close(connection_fd, 404, "The requested URL was not found on this server\n");
        return;
    }

    if (strncmp(read_buffer + strlen(path_prefix), query_prefix, strlen(query_prefix)) != 0) {
        respond_and_close(connection_fd, 400, "Failed to find query");
        return;
    }

    char* query_start = read_buffer + strlen(path_prefix) + strlen(query_prefix);
    int a, b;
    if (sscanf(query_start, "%d+%d", &a, &b) != 2) {
        respond_and_close(connection_fd, 400, "Failed to parse as a valid expression\n");
        free(read_buffer);
        return;
    }

    int result = a + b;
    char body[64];
    snprintf(body, sizeof(body), "%d\n", result);
    respond_and_close(connection_fd, 200, body);
    free(read_buffer);
}