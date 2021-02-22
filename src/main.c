#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include "sntp.h"

noreturn void print_usage_and_exit(char **argv) {
    fprintf(stderr, "Usage: %s [-v] [SERVER [PORT]]\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    char *server = "pool.ntp.org";
    char *port = "123";
    int debug = 0;

    NtpResult result = {0};

    int got_server = 0;
    int got_port = 0;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] =='-') {
            if(argv[i][1] == 'v') {
                debug = 1;
            } else {
                print_usage_and_exit(argv);
            }
        } else {
            if (!got_server) {
                server = argv[i];
                got_server = 1;
            } else if (!got_port) {
                port = argv[i];
                got_port = 1;
            } else {
                print_usage_and_exit(argv);
            }
        }
    }

    sntp_request(&result, debug, server, port);
    printf("%u.%u\n", result.timestamp.seconds, result.timestamp.seconds_fraction);
}
