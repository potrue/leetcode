#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void respond_and_close(int connection_fd, int status_code, const char* status_msg, const char* body) {
    char response[RESPONSE_BUFFER_SIZE];

    int response_length = snprintf(response, sizeof(response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s",
        status_code, status_msg, strlen(body), body
    );

    if (response_length >= sizeof(response)) {
        response_length = sizeof(response) - 1; // 終端文字を入れないようにする
    }

    if (write(connection_fd, response, response_length) < 0) {
        perror("write");
    }

    close(connection_fd);
}

void handle_connection(int connection_fd) {
    char buffer[READ_BUFFER_SIZE] = {0};
    if (read(connection_fd, buffer, sizeof(buffer)) < 0) {
        perror("read");
        respond_and_close(connection_fd, 500, "Internal Server Error",
                          "Something went wrong\n");
        return;
    };

    const char locator[] = "?query=";
    char* locator_start = strstr(buffer, locator);
    if (!locator_start) {
        respond_and_close(connection_fd, 400, "Bad Request",
                          "Failed to parse as a valid expression\n");
        return;
    }

    char* query_start = locator_start + strlen(locator);
    int a, b;
    int sscanf_result = sscanf(query_start, "%d+%d", &a, &b);
    if ((sscanf_result) != 2) {
        respond_and_close(connection_fd, 400, "Bad Request",
                          "Failed to parse as a valid expression\n");
        return;
    }

    int result = a + b;
    char body[32];
    snprintf(body, sizeof(body), "%d\n", result);
    respond_and_close(connection_fd, 200, "OK", body);
}