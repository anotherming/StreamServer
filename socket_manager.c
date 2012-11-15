

#include "socket_manager.h"

int send_request (int sd, request_t *request);
int read_request (int sd, request_t *request);
int send_response (int sd, response_t *response);
int read_response (int sd, response_t *response);


int init_socket_server (socket_manager_t *manager, int port) {
	assert (manager != NULL);

	// create sd
	manager->port = port;
	manager->main_sd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	assert (manager->main_sd > 0);

	// prepare for bind
	memset ((void *)&manager->name, 0, sizeof (struct sockaddr_in));
	manager->name.sin_family = AF_INET;
	manager->name.sin_port = htons (port);
	manager->name.sin_addr.s_addr = htonl (INADDR_ANY);

	// bind
	int status = bind (manager->main_sd, (struct sockaddr *)&manager->name, sizeof (struct sockaddr_in));
	assert (status == 0);

	// listen
	status = listen (manager->main_sd, MAX_CONN);
	assert (status == 0);

	// epoll
	manager->epoll_descriptor = epoll_create (MAX_EVENTS);
    assert (manager->epoll_descriptor >= 0);

    struct epoll_event event = {
    	.events = EPOLLIN,
    	.data.fd = manager->main_sd
    };
    status = epoll_ctl (manager->epoll_descriptor, EPOLL_CTL_ADD, manager->main_sd, &event);
    assert (status == 0)

	return 0;
}


int _run_server (socket_manager_t *manager) {
	assert (manager != NULL);

	struct epoll_event events[MAX_EVENTS];

	int num = epoll_wait (manager->epoll_descriptor, events, MAX_EVENTS, -1);
	int i;
	for (i = 0; i < num; i++) {
		// server
		if (events[i].data.fd == manager->main_sd) {

			// max connection reached
			if (manager->count >= MAX_CONN)
				assert (false);

			// accept
			int sd = accept (manager->main_sd, (struct sockaddr *)&manager->name, sizeof (struct sockaddr_in));
			manager->client_sds[manager->count] = sd;
			assert (sd >= 0);

			// add to epoll
			struct epoll_event event = {
				.events = EPOLLIN,
				.data.fd = sd
			};

			int status = epoll_ctl (manager->epoll_descriptor, EPOLL_CTL_ADD, sd, &event);
			assert (status == 0);

		// client EPOLLIN
		} else if (events[i].events & EPOLLIN) {
			
		}
	}
}




int inline read_request (int sd, request_t *request) {
	assert (request != NULL);

	int len = read (sd, request, sizeof (request_t));
	if (len == 0)
		close (sd);
	assert (len == sizeof (request_t));

	return len;
}

int inline send_request (int sd, request_t *request) {
	assert (request != NULL);

	int len = write (sd, request, sizeof (request_t));
	return len;
}