


int send_request (int sd, request_t *request);
int read_request (int sd, request_t *request);
int send_response (int sd, response_t *response);
int read_response (int sd, response_t *response);

int init_socket_server (scoket_manager_t *manager, int port);
int init_socket_client (socket_manager_t *manager, int port, char *host);

#define MAX_HOSTNAME 128
#define MAX_CONN 1024

typedef struct socket_manager_t {
	int epoll_descriptor;

	int port;
	char host[MAX_HOSTNAME];

	int main_sd;
	int client_sds[MAX_CONN];
	int count;

	bool shutdown;

	int (*handle) (socket_manager_t *manager, int sd, void *data, int len);
	int (*write) (socket_manager_t *manager, int sd, void *data, int len);

	int (*run) (socket_manager_t *manager);
} socket_manager_t;



//_read // for internal use
/*
	read...
	handle ()... // user (I) will fill this handle with actual code. Like assign tasks to a worker thread.
*/


//_write
/*
	write (sd ....)
*/


//_run_server : init_socket_server will assign this to the "run"
/*
while (shutdown == false) {
	struct epoll_event events;

	int num = epoll_wait (... &events ...);

	for (i = 0; i < num; i++) {
		if (events[i].data.fd == main_sd)
			accept...
			add new sd to epoll
			...
		else 
			_read ...
	}

}
*/


// _run_client : init_socket_client will assign this to the "run"





