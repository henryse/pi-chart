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

#include <stdbool.h>
#include <time.h>
#include <memory.h>
#include "pi_chart_settings.h"
#include "version_config.h"
#include "pi_string.h"

bool run_as_daemon = false;
pid_t process_id = 0;
unsigned short server_port = 8090;
bool service_running = false;
char *default_directory = ".";
pi_string_ptr file_directory = NULL;

const char *get_pi_chart_version() {
    return PI_CHART_VERSION;
}

bool get_run_as_daemon() {
    return run_as_daemon;
}

void set_run_as_daemon(bool value) {
    run_as_daemon = value;
}

pid_t get_process_id() {
    return process_id;
}

void set_process_id(pid_t value) {
    process_id = value;
}

bool get_log_mode() {
    return true;
}

unsigned short get_server_port() {
    return server_port;
}

void set_server_port(unsigned short value) {
    server_port = value;
}

bool get_service_running() {
    return service_running;
}

void set_service_running(bool value) {
    service_running = value;
}

void set_file_directory(char *directory) {
    if (NULL == file_directory) {
        file_directory = pi_string_new(strlen(directory));
    }

    pi_string_reset(file_directory);
    pi_string_append_str(file_directory, directory);
}

const char *get_file_directory() {
    if (NULL == file_directory) {
        return default_directory;
    }

    return pi_string_c_string(file_directory);
}
