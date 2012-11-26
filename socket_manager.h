#ifndef _SOCKET_H
#define _SOCKET_H

#include "bool.h"
#include "server.h"
#include "client.h"
//#include <sys/socket.h>
//#include <sys/types.h>
#include <netinet/in.h>


#define MAX_HOSTNAME 128
#define MAX_CONN 1024
#define MAX_BUFFER 1024
#define MAX_EVENTS 1024

#define ERR_AGAIN -9

// typedef struct {
// 	char buffer[MAX_BUFFER];
// 	int head, tail, len;
// } socket_buffer_t;

//setsockopt(s,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));


struct server_t;
struct client_t;

typedef struct socket_manager_t {
	int epoll_descriptor;

	int port;
	char host[MAX_HOSTNAME];

	int main_sd;
	int client_sds[MAX_CONN];
	int count;

	bool shutdown;

	struct sockaddr_in name;
	struct server_t *server;
	struct client_t *client;

	int (*handle) (struct socket_manager_t *manager, int sd, void *data, int len, void *args);
	int (*write) (struct socket_manager_t *manager, int sd, void *data, int len, void *args);

	int (*run) (struct socket_manager_t *manager);
} socket_manager_t;

int init_socket_server (socket_manager_t *manager, int port);
int init_socket_client (socket_manager_t *manager, int port, char *host);

int destroy_socket_manager (socket_manager_t *manager);

#endif