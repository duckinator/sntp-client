#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h> /* for close() */
#include <stdlib.h>

int socket_create(const char *server, char *port) {
    int sock = -1;
    struct addrinfo hints = {0};
    struct addrinfo *result = NULL;

    hints.ai_family = AF_UNSPEC; // Allow IPv4 or IPv6.
    hints.ai_socktype = SOCK_DGRAM; // UDP.
    hints.ai_flags = AI_PASSIVE; // ???
    hints.ai_flags = 0;
    hints.ai_protocol = 0; // Any protocol.

    int gaddr_error = getaddrinfo(server, port, &hints, &result);
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
        fprintf(stderr, "Could not connect to address %s on port %s.", server, port);
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
