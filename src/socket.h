#ifndef PUPPY_SOCKETS_H
#define PUPPY_SOCKETS_H

#include <stddef.h>

int socket_create(const char *server, int port);
void socket_write(int sock, void *buf, size_t nbyte);
void socket_read(int sock, void *buf, size_t nbyte);

#endif
