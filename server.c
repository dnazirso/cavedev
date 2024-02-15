#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 3000
#define BUFFER_SIZE 1024
#define WEB_ROOT "./public"

void send_response(int client_socket, const char *response) {
    send(client_socket, response, strlen(response), 0);
}

const char *get_content_type(const char *file_path) {
    const char *ext = strrchr(file_path, '.');
    if (ext != NULL) {
        if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) {
            return "text/html";
        } else if (strcmp(ext, ".css") == 0) {
            return "text/css";
        } else if (strcmp(ext, ".js") == 0) {
            return "application/javascript";
        }
    }
    return "text/plain";
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    recv(client_socket, buffer, BUFFER_SIZE, 0);

    char method[16], path[256];
    sscanf(buffer, "%15s %255s", method, path);

    if (path[strlen(path) - 1] == '/') {
        strcat(path, "index.html");
    }

    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s%s", WEB_ROOT, path);

    FILE *file = fopen(file_path, "r");
    if (file != NULL) {
        const char *content_type = get_content_type(file_path);

        char response_header[BUFFER_SIZE];
        snprintf(response_header, sizeof(response_header), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", content_type);
        send_response(client_socket, response_header);

        char file_buffer[BUFFER_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), file)) > 0) {
            send(client_socket, file_buffer, bytes_read, 0);
        }
        fclose(file);
    } else {
        const char *not_found_response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
        send_response(client_socket, not_found_response);
    }

    close(client_socket);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        handle_client(client_socket);
    }

    return 0;
}