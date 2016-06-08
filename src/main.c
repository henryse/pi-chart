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

#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <memory.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include "pi_chart_settings.h"
#include "pi_utils.h"
#include "pi_chart_server.h"
#include "pi_chart_gpio.h"

void usage(const char *program) {
    fprintf(stdout, "Version: %s\n", get_pi_chart_version());
    fprintf(stdout, "Usage:     %s --port=PORT_NUMBER --daemon=true/false \n", program);
    fprintf(stdout, "Example:   %s --port=8080 --daemon=true \n\n", program);
    fprintf(stdout, "Simple Raspberry PI monitoring tool.\n\n");
    fprintf(stdout, "     daemon     run as daemon, default: %s\n", get_run_as_daemon() ? "true" : "false");
    fprintf(stdout, "     port       port to listen to, default: %d\n", (int) get_server_port());
    fprintf(stdout, "     directory  directory to read files from: %s\n", get_file_directory());
    fprintf(stdout, "     help       get this help message\n");
}

bool parse_arguments(int argc, char *argv[]) {

    static struct option long_options[] =
            {
                    {"daemon",    optional_argument, 0, 'd'},
                    {"port",      optional_argument, 0, 'p'},
                    {"directory", optional_argument, 0, 'f'},
                    {"help",      optional_argument, 0, '?'},
                    {0, 0,                           0, 0}
            };

    int option_index = 0;
    int c = 0;

    do {
        c = getopt_long(argc, argv, "?p:d:f:", long_options, &option_index);

        switch (c) {
            case -1:
                // Ignore this one.
                break;

            case 'p':
                set_server_port((unsigned short) atol(optarg));
                fprintf(stdout, "\nPort to listen on %u\n", get_server_port());
                break;

            case 'd':
                set_run_as_daemon(strcmp(optarg, "true") == 0);
                fprintf(stdout, "\nRun as daemon %s\n", get_run_as_daemon() ? "true" : "false");
                break;

            case 'f':
                set_file_directory(optarg);
                fprintf(stdout, "\nDirectory to read files from %s\n", get_file_directory());
                break;

            case '?':
            default:
                usage("pi-chart");
                return false;
        }
    } while (c != -1);

    return true;
}

void service_stop() {
    set_service_running(false);

    close_logs();
}

void signal_shutdown(int value) {
    INFO_LOG("Shutting down the service, signal: %d", value);
    service_stop();
    exit(value);
}

void signal_SIGPIPE(int value) {
    INFO_LOG("SIGPIPE failure: %d", value);
    service_stop();
    exit(value);
}

void pi_chart_process_setup() {
    signal(SIGABRT, signal_shutdown);
    signal(SIGFPE, signal_shutdown);
    signal(SIGILL, signal_shutdown);
    signal(SIGINT, signal_shutdown);
    signal(SIGSEGV, signal_shutdown);
    signal(SIGTERM, signal_shutdown);
    signal(SIGPIPE, signal_SIGPIPE);
}

void fork_process() {
    if (get_run_as_daemon()) {
        // Create child process
        //
        pid_t process_id = fork();

        // Indication of fork() failure
        //
        if (process_id < 0) {

            ERROR_LOG("Forking the process failed.");
            // Return failure in exit status
            exit(1);
        }

        // PARENT PROCESS. Need to kill it.
        //
        if (process_id > 0) {
            ERROR_LOG("Process ID of child process %d", process_id);

            // return success in exit status
            //
            exit(0);
        }

        // Unmask the file mode
        //
        umask(0);

        //set new session
        //
        pid_t sid = setsid();
        if (sid < 0) {
            ERROR_LOG("Failed to obtain process id");
            // Return failure
            exit(1);
        }

        set_process_id(sid);

        // Daemon Process ID
        //
        INFO_LOG("Starting process %d", get_process_id());

        // Change the current working directory to root.
        //
        chdir("/");

        // Close stdin. stdout and stderr
        //
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    // Setup base service
    //
    pi_chart_process_setup();
}

int main(int argc, const char *argv[]) {

    if (parse_arguments(argc, (char **) argv)) {

        fork_process();

        setup_wring_pi();

        create_logs();

        set_service_running(true);

        pi_chart_service_start();

        printf("\n\nPress q [enter] to quit...\n\n");

        int keystroke = 0;
        while (keystroke != 'q') {
            keystroke = getchar();
        }

        service_stop();
    }

    return 0;
}
