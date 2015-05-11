#include "ipc.h"
#include "../util.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>     /* close() */
#include <sys/socket.h> /* socket(), AF_UNIX */
#include <sys/un.h>     /* struct sockaddr_un */

int
ipc_accept(int csock)
{
	int sock;

	sock = accept(csock, NULL, NULL);
	if (sock < 0)
		ERROR("IPC could not accept connection: %s", strerror(errno));
	return sock;
}

int
ipc_init(char const *path)
{
	int i, sock;
	size_t plen;
	struct sockaddr_un addr;

	plen = sizeof(addr.sun_path) - 1;
	if (strlen(path) > plen)
		WARN("path '%s' is too long, will truncate", path);

	/* open unix socket */
	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		ERROR("failed to open socket: %s", strerror(errno));
		goto ipc_new_fail_socket;
	}

	/* bind path ("address") to socket */
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, plen);
	i = bind(sock, (struct sockaddr *) &addr, sizeof(addr));
	if (i < 0) {
		ERROR("failed to bind address to socket: %s", strerror(errno));
		goto ipc_new_fail_bind;
	}

	/* listen on socket */
	i = listen(sock, 5);
	if (i < 0) {
		ERROR("failed to listen on socket: %s", strerror(errno));
		goto ipc_new_fail_listen;
	}
	return sock;

 ipc_new_fail_listen:
 ipc_new_fail_bind:
	close(sock);
 ipc_new_fail_socket:
	return -1;
}

int
ipc_read(int sock, size_t buflen, char *buf)
{
	ssize_t r;

	r = read(sock, buf, buflen);
	if (r == 0)
		DEBUG("IPC:%d closed connection", sock);
	if (r < 0)
		ERROR("IPC:%d failed to read client data: %s",
		      sock, strerror(errno));
	return (int) r;
}

int
ipc_send(int sock, size_t len, char const *data)
{
	ssize_t w;

	w = write(sock, data, len);
	if (w < (ssize_t) len) {
		ERROR("IPC:%d failed to accept data%s%s",
		      sock, w < 0 ? " " : "", w < 0 ? strerror(errno) : "");
		return -1;
	}
	return 0;
}

void
ipc_term(int socket)
{
	close(socket);
}
