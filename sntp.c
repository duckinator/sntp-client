#!/usr/bin/env cci
// cci: -Wall -Weverything -pedantic-errors -Wno-missing-prototypes

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h> /* for close() */
#include <stdlib.h>
#include <string.h>

int socket_create(const char *server, int port) {
    int sock = -1;
    struct addrinfo hints = {0};
    struct addrinfo *result = NULL;
    char port_s[10] = "";
    snprintf(port_s, sizeof(port_s), "%u", (unsigned int)port);

    hints.ai_family = AF_UNSPEC; // Allow IPv4 or IPv6.
    hints.ai_socktype = SOCK_DGRAM; // UDP.
    hints.ai_flags = AI_PASSIVE; // ???
    hints.ai_flags = 0;
    hints.ai_protocol = 0; // Any protocol.

    int gaddr_error = getaddrinfo(server, port_s, &hints, &result);
    if (gaddr_error != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gaddr_error));
        exit(EXIT_FAILURE);
    }

    // Try each addresses returned by getaddrinfo() until bind() succeeds.
    // If socket() or bind() fail, close the socket and try the next address.
    struct addrinfo *rp = NULL;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == -1) {
            continue;
        }

        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
            // Success.
            break;
        }

        // If we failed, clean up.
        close(sock);
    }

    freeaddrinfo(result); // Not needed anymore.

    if (rp == NULL) {
        // Every address failed.
        fprintf(stderr, "Could not connect to address %s on port %s.", server, port_s);
        exit(EXIT_FAILURE);
    }

    return sock;
}

void socket_write(int sock, void *buf, size_t nbyte) {
    ssize_t result = write(sock, buf, nbyte);

    if (result == -1) {
        fprintf(stderr, "write: failed to write to socket\n");
        exit(EXIT_FAILURE);
    } else if ((size_t)result < nbyte) {
        fprintf(stderr, "write: only wrote part of buffer\n");
        exit(EXIT_FAILURE);
    }
}

void socket_read(int sock, void *buf, size_t nbyte) {
    ssize_t result = read(sock, buf, nbyte);
    if (result == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
}

#define BUF_SIZE (64 / 8)

typedef struct {
    // LI, VN, and Mode are 3 different things.
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint64_t ref_timestamp;
    uint64_t origin_timestamp;
    uint64_t recv_timestamp;
    uint64_t tx_timestamp;

    uint32_t key_id; // optional
    uint64_t msg_digest_1; // optional
    uint64_t msg_digest_2; // optional
} NtpPacket;

int main() {
    char *server = "pool.ntp.org";
    int port = 123;

    char *request = "Hello!";
    char response[BUF_SIZE] = {0};

    int sock = socket_create(server, port);

    socket_write(sock, request, strlen(request) + 1);
    socket_read(sock, response, BUF_SIZE);

    printf("buf = '%s'\n", response);

    return 0;
}
