/**********************************************************************
//    Copyright (c) 2016 Henry Seurer & Samuel Kelly
//
//    Permission is hereby granted, free of charge, to any person
//    obtaining a copy of this software and associated documentation
//    files (the "Software"), to deal in the Software without
//    restriction, including without limitation the rights to use,
//    copy, modify, merge, publish, distribute, sublicense, and/or sell
//    copies of the Software, and to permit persons to whom the
//    Software is furnished to do so, subject to the following
//    conditions:
//
//    The above copyright notice and this permission notice shall be
//    included in all copies or substantial portions of the Software.
//
//    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
//    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
//    OTHER DEALINGS IN THE SOFTWARE.
//
**********************************************************************/

#include <ntsid.h>

#ifndef __MACH__
#define _POSIX_C_SOURCE 200809L
#include <strings.h>
#endif

#ifndef __unused
#define __unused
#endif

#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "pi_utils.h"
#include "pi_chart_settings.h"
#include "pi_string.h"
#include "pi_strmap.h"
#include "pi_template_generator.h"
#include "pi_chart_gpio.h"

size_t http_read_line(int socket, pi_string_ptr buffer) {
    if (NULL == buffer) {
        return 0;
    }

    // Free the buffer because the expectation is that we are reading in a
    // whole new line.
    //
    pi_string_reset(buffer);

    // Now we read in the data:
    //
    char c = '\0';

    while (c != '\n') {
        ssize_t n = recv(socket, &c, 1, 0);

        if (n > 0) {
            if (c == '\r') {
                n = recv(socket, &c, 1, MSG_PEEK);

                if ((n > 0) && (c == '\n')) {
                    recv(socket, &c, 1, 0);
                }
                else {
                    c = '\n';
                }
            }
            pi_string_append_char(buffer, c);
        }
        else {
            c = '\n';
        }
    }

    return pi_string_c_string_length(buffer);
}



// The following symbols should be supported:
//      gpoi.#           - where the # is the pin number (1(true), 0(false))
//      pi.memory        - total memory for the computer
//      pi.memory_free   - total free memory on the computer
//      process.name     - if the process with the "name" exits
//                          string:  return pid
//                          boolean: return true if running and false it not running
bool function_string(void __unused *context_ptr,
                     const char *symbol,
                     pi_string_ptr buffer) {

    if (strncmp(symbol, "gpio.", 5) == 0) {
        int pin = atoi(&symbol[5]);
        pi_string_append_str(buffer, gpio_get_str((unsigned char) pin));
        return true;
    }

    return false;
}

bool function_boolean(void __unused *context_ptr,
                      const char *symbol,
                      bool *value) {

    if (strncmp(symbol, "gpio.", 5) == 0) {
        if (value) {
            int pin = atoi(&symbol[5]);
            *value = gpio_get_int((unsigned char) pin) != 0;
        }
        return true;
    }

    return false;
}

void http_html_clean_string(pi_string_ptr request_path) {
    pi_string_ptr clean_buffer = pi_string_new(pi_string_c_string_length(request_path));

    for (int i = 0; i < pi_string_c_string_length(request_path); i++) {
        char c = pi_string_c_string(request_path)[i];
        switch (c) {
            case '.':
            case '\\':
            case '?':
            case '!':
                // Don't want any of these....
                break;
            default:
                pi_string_append_char(clean_buffer, c);
                break;
        }
    }

    pi_string_reset(request_path);
    pi_string_append_str(request_path, pi_string_c_string(clean_buffer));
    pi_string_delete(clean_buffer, true);
}

size_t http_html_file_size(FILE *file_p) {
    if (NULL == file_p) {
        return 0;
    }

    fseek(file_p, 0L, SEEK_END);
    long file_size = ftell(file_p);
    fseek(file_p, 0L, SEEK_SET);

    return (size_t) file_size;
}

bool http_html_monitor_page(pi_string_ptr response,
                            pi_strmap_ptr __unused headers,
                            pi_string_ptr request_path) {

    bool success = false;

    // Strip out any bad characters.
    //
    http_html_clean_string(request_path);

    // Check to see if we should load the default html page = index.html
    //
    if (strcmp(pi_string_c_string(request_path), "/") == 0) {
        pi_string_append_str(request_path, "index");
    }

    // Build file name
    pi_string_ptr source_file = pi_string_new(pi_string_c_string_length(request_path) + 32);
    pi_string_sprintf(source_file, "%s%s.html", get_file_directory(), pi_string_c_string(request_path));

    FILE *file_p = fopen(pi_string_c_string(source_file), "r");

    if (NULL != file_p) {
        size_t file_size = http_html_file_size(file_p);

        // Read in the file:
        //
        char *file_contents = alloca(file_size);
        memory_clear(file_contents, file_size);

        if (fread(file_contents, file_size, sizeof(char), file_p) != 0) {
            // Setup buffers.
            //
            pi_string_ptr input_buffer = pi_string_new(file_size + 1);
            pi_string_append_str_length(input_buffer, file_contents, file_size);

            pi_string_ptr response_body = pi_string_new(file_size + 1);

            pi_template_generate_output(input_buffer, response_body, NULL, function_string, function_boolean);

            pi_string_sprintf(response, "HTTP/1.0 200 OK\r\n");
            pi_string_sprintf(response, "Server: %s\r\n", get_pi_chart_version());
            pi_string_sprintf(response, "Content-Type: text/html\r\n");
            pi_string_sprintf(response, "Connection: close\r\n");
            pi_string_sprintf(response, "Content-Length: %d\r\n", pi_string_c_string_length(response_body));
            pi_string_sprintf(response, "\r\n%s", pi_string_c_string(response_body));

            pi_string_delete(response_body, true);
            pi_string_delete(input_buffer, true);
            success = true;
        }
    }

    pi_string_delete(source_file, true);

    return success;
}


void http_output_health_check(pi_string_ptr response) {

    pi_string_ptr response_body = pi_string_new(256);

    pi_string_sprintf(response_body, "{\"status\":\"UP\"");

    pi_string_sprintf(response, "HTTP/1.0 200 OK\r\n");
    pi_string_sprintf(response, "Server: %s\r\n", get_pi_chart_version());
    pi_string_sprintf(response, "Transfer-Encoding: Identity\r\n");
    pi_string_sprintf(response, "Content-Type: application/json;charset=UTF-8\r\n");
    pi_string_sprintf(response, "Connection: close\r\n");
    pi_string_sprintf(response, "Content-Length: %d\r\n", pi_string_c_string_length(response_body));
    pi_string_sprintf(response, "\r\n%s", pi_string_c_string(response_body));

    pi_string_delete(response_body, true);
}

void http_output_build_info(pi_string_ptr response) {

    pi_string_ptr response_body = pi_string_new(256);

    pi_string_sprintf(response_body, "{\"version\":\"%s\", \"name\":\"pi-chart\"}", get_pi_chart_version());

    pi_string_sprintf(response, "HTTP/1.0 200 OK\r\n");
    pi_string_sprintf(response, "Server: %s\r\n", get_pi_chart_version());
    pi_string_sprintf(response, "Content-Type: application/json;charset=UTF-8\r\n");
    pi_string_sprintf(response, "Connection: close\r\n");
    pi_string_sprintf(response, "Content-Length: %d\r\n", pi_string_c_string_length(response_body));
    pi_string_sprintf(response, "\r\n%s", pi_string_c_string(response_body));

    pi_string_delete(response_body, true);
}

void http_not_found(pi_string_ptr response) {

    pi_string_ptr response_body = pi_string_new(256);

    pi_string_sprintf(response_body, "<HTML><TITLE>Not Found</TITLE>\r\n");
    pi_string_sprintf(response_body, "<BODY><P>The server could not fulfill\r\n");
    pi_string_sprintf(response_body, "your request because the resource specified\r\n");
    pi_string_sprintf(response_body, "is unavailable or nonexistent.</P>\r\n");
    pi_string_sprintf(response_body, "</BODY></HTML>\r\n");
    pi_string_sprintf(response_body, "\r\n");

    pi_string_sprintf(response, "HTTP/1.0 404 NOT FOUND\r\n");
    pi_string_sprintf(response, "Server: %s\r\n", get_pi_chart_version());
    pi_string_sprintf(response, "Content-Type: text/html\r\n");
    pi_string_sprintf(response, "Connection: close\r\n");
    pi_string_sprintf(response, "Content-Length: %d\r\n", pi_string_c_string_length(response_body));
    pi_string_sprintf(response, "\r\n%s", pi_string_c_string(response_body));

    pi_string_delete(response_body, true);
}

void http_output_response(pi_string_ptr request_path, pi_strmap_ptr headers, pi_string_ptr response) {

    if (request_path && 0 == strncmp(pi_string_c_string(request_path), "/health", strlen("/health"))) {
        // Do Health Checks
        //
        http_output_health_check(response);
    }
    else if (request_path && 0 == strncmp(pi_string_c_string(request_path), "/buildInfo", strlen("/buildInfo"))) {
        // Output Build information
        //
        http_output_build_info(response);
    }
    else {
        if (!http_html_monitor_page(response, headers, request_path)) {
            http_not_found(response);
        }
    }
}

void http_send_response(int socket, pi_string_ptr response) {
    if (NULL != response) {
        if (pi_string_c_string_length(response) > 0) {
            send(socket,
                 pi_string_c_string(response),
                 pi_string_c_string_length(response), 0);
        }
    }
}

int pi_chart_service_connection() {

    struct addrinfo hints;
    memory_clear(&hints, sizeof hints);

    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    char port_string[16];   // needs to fit a short
    memory_clear(port_string, sizeof port_string);
    snprintf(port_string, sizeof port_string, "%d", get_server_port());

    struct addrinfo *response = NULL;
    getaddrinfo(NULL, port_string, &hints, &response);

    int socket_fd = socket(response->ai_family, response->ai_socktype, response->ai_protocol);

    if (socket_fd == -1) {
        ERROR_LOG("Unable to create socket, this is either a network issue where the port %"
                          " is already in use or a bug in the service.", get_server_port());
    }
    else {
        // The setsockopt() function is used to allow the local address to
        // be reused when the server is restarted before the required wait
        // time expires.
        //
        int option_one = 1;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,
                       (char *) &option_one, sizeof(option_one)) < 0) {
            ERROR_LOG("Setsockopt(SO_REUSEADDR) failed, this is either"
                              " a network issue or a bug in the service.");
        }

        if (bind(socket_fd, response->ai_addr, response->ai_addrlen) < 0) {
            ERROR_LOG("Bind failed on socket %d, this is either a network "
                              "issue or a bug in the service", socket_fd);
            close(socket_fd);
            socket_fd = -1;
        }

        // The backlog argument (5) defines the maximum length to which the queue of pending connections for
        // sockfd may grow. If a connection request arrives when the queue is full, the client may receive an
        // error with an indication of ECONNREFUSED or, if the underlying protocol supports
        // retransmission, the request may be ignored so that a later reattempt at connection succeeds.
        //

        if (listen(socket_fd, 5) < 0) {
            ERROR_LOG("listen debug failed");
            close(socket_fd);
            socket_fd = -1;
        }
    }

    return socket_fd;
}

// List of methods
//
typedef enum {
    http_invalid,
    http_get,
    http_post,
    http_delete,
    http_put,
    http_options,
    http_head,
    http_trace,
    http_connect
} http_method_t;

http_method_t http_map_string_to_method(pi_string_ptr request_buffer) {
    http_method_t result = http_invalid;
    const char *method = pi_string_c_string(request_buffer);

    if (0 == strncasecmp(method, "GET", 3)) {
        result = http_get;
    }
    else if (0 == strncasecmp(method, "POST", 4)) {
        result = http_post;
    }
    else if (0 == strncasecmp(method, "PUT", 3)) {
        result = http_put;
    }
    else if (0 == strncasecmp(method, "DELETE", 6)) {
        result = http_delete;
    }
    else if (0 == strncasecmp(method, "OPTIONS", 7)) {
        result = http_options;
    }
    else if (0 == strncasecmp(method, "HEAD", 4)) {
        result = http_head;
    }
    else if (0 == strncasecmp(method, "TRACE", 5)) {
        result = http_trace;
    }
    else if (0 == strncasecmp(method, "CONNECT", 7)) {
        result = http_connect;
    }

    return result;
}

pi_string_ptr http_parse_path(pi_string_ptr request_buffer) {
    const char *query = pi_string_c_string(request_buffer);

    // Skip Method
    //
    while (*query != '\0' && *query != ' ' && *query != '\t') {
        ++query;
    }

    // Skip Spaces
    //
    while (*query != '\0' && (*query == ' ' || *query == '\t')) {
        ++query;
    }

    pi_string_ptr request_path = pi_string_new(256);

    // Extract the path
    //
    while (*query != '\0' && *query != '?' && *query != ' ' && *query != '\t') {
        pi_string_append_char(request_path, *query);
        query++;
    }

    return request_path;
}

char *clean_string(char *value) {
    char *zap = strrchr(value, '\n');
    if (zap) *zap = 0;
    zap = strrchr(value, '\r');
    if (zap) *zap = 0;

    if (*value == ' ') {
        value++;
    }

    return value;
}

size_t parse_headers(int client_socket, pi_strmap_ptr headers) {
    size_t content_size = 0;

    // Read the headers...
    //
    pi_string_ptr string_buffer_ptr = pi_string_new(256);

    while (http_read_line(client_socket, string_buffer_ptr)) {
        // Find the key and the value
        //
        char *key = pi_string_c_string(string_buffer_ptr);
        char *value = pi_string_c_string(string_buffer_ptr);

        // Look for the ':'
        //
        for (; *value && *value != ':'; value++);

        // Get the "Value
        //
        if (*value == ':') {
            *value = 0;
            value++;
        }

        key = clean_string(key);
        value = clean_string(value);

        if (*key && *value) {
            pi_strmap_put(headers, key, value);
        }

        // Check to see if we have hit the end...
        //
        if (*key == '\0') {
            // We hit the end!
            //
            break;
        }

        // OK look of content length
        //
        if (0 == strncmp(key, "Content-Length", 32)) {
            content_size = (size_t) atol(value);
        }
    }

    pi_string_delete(string_buffer_ptr, true);
    return content_size;
}


void *pi_server_thread(void __unused *arg) {

    INFO_LOG("Starting server thread on port %hu", get_server_port());

    if (get_server_port()) {
        int socket_fd = pi_chart_service_connection();

        if (-1 != socket_fd) {
            INFO_LOG("[INFO] Service has taking the stage on port %d", get_server_port());

            pi_string_ptr request_buffer = NULL;

            while (get_service_running()) {
                request_buffer = pi_string_new(1024);

                struct sockaddr_in sockaddr_client;
                socklen_t sockaddr_client_length = sizeof(sockaddr_client);
                memory_clear(&sockaddr_client, sockaddr_client_length);

                int client_socket = accept(socket_fd, (struct sockaddr *) &sockaddr_client, &sockaddr_client_length);

                http_read_line(client_socket, request_buffer);

                http_method_t method = http_map_string_to_method(request_buffer);

                pi_string_ptr request_path = http_parse_path(request_buffer);

                pi_string_ptr response_buffer = pi_string_new(1024);

                pi_strmap_ptr headers = pi_strmap_new(32);

                parse_headers(client_socket, headers);

                switch (method) {
                    case http_get:
                        http_output_response(request_path, headers, response_buffer);
                        break;
                    default:
                        http_not_found(response_buffer);
                        break;
                }

                http_send_response(client_socket, response_buffer);

                close(client_socket);

                pi_string_delete(response_buffer, true);
                pi_string_delete(request_path, true);
                pi_strmap_delete(headers);
            }
            pi_string_delete(request_buffer, true);

            close(socket_fd);
        }
        else {
            ERROR_LOG("[ERROR] Service was unable to take the stage on port %d", get_server_port());
        }
    }

    return NULL;
}

pthread_t g_server_thread_id = 0;

void pi_chart_service_start() {
    if (get_server_port()) {
        pthread_create(&g_server_thread_id, NULL, &pi_server_thread, NULL);
    }
}

