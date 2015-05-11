#ifndef _API_IPC_H
#define _API_IPC_H

#include <stdlib.h>

int ipc_accept(int csock);
int ipc_init(char const *path);
int ipc_read(int sock, size_t buflen, char *buf);
int ipc_send(int sock, size_t len, char const *data);
void ipc_term(int socket);

#endif /* _API_IPC_H */
