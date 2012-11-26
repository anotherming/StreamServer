
#include "socket_manager.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <netdb.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>

int send_request (int sd, request_t *request);
int read_request (int sd, request_t *request);
int send_response (int sd, response_t *response);
int read_response (int sd, response_t *response);

int _run_client (socket_manager_t *manager);
int _run_server (socket_manager_t *manager);

void inline _set_nonblock (int sock) {
	int status;
	status = fcntl (sock, F_GETFL, 0);
	assert (status >= 0);

	status = fcntl (sock, F_SETFL, status | O_NONBLOCK);
	assert (status >= 0);
}

int init_socket_client (socket_manager_t *manager, int port, char *host) {
	assert (manager != NULL);
	assert (port > 0);
	assert (host != NULL);

    manager->shutdown = false;


	zlog_category_t *category = zlog_get_category ("socket");
	zlog_info (category, "Initializing client socket manager with %s:%d.", host, port);

	// create sd
	zlog_debug (category, "Creating socket descriptor.");
	manager->port = port;
	strcpy (manager->host, host);
	manager->main_sd = socket (AF_INET, SOCK_STREAM, 0);
	assert (manager->main_sd > 0);

	// resolve host address
	zlog_debug (category, "Resolving host name.");
	struct hostent *hent;
	hent = gethostbyname (host);
	assert (hent != NULL);

	// prepare for connect
	memset ((void *)&manager->name, 0, sizeof (struct sockaddr_in));
	manager->name.sin_family = AF_INET;
	manager->name.sin_port = htons (port);
    bcopy (hent->h_addr, &manager->name.sin_addr, hent->h_length);

    // connect
    zlog_debug (category, "Connecting to server.");
    int status = connect (manager->main_sd, (struct sockaddr *)&manager->name, sizeof(struct sockaddr_in));
    assert (status >= 0);

    // change buffer
    int n = 57654 * 100;
    setsockopt (manager->main_sd, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n));

    status = fcntl (manager->main_sd, F_GETFL, 0);
	status = fcntl (manager->main_sd, F_SETFL, ~O_NONBLOCK);

    // hook
    manager->run = _run_client;
	return 0;
}

int init_socket_server (socket_manager_t *manager, int port) {
	assert (manager != NULL);

	zlog_category_t *category = zlog_get_category ("socket");
	zlog_info (category, "Initializing server socket manager at %d.", port);

    manager->shutdown = false;

	// create sd
	zlog_debug (category, "Creating socket descriptor.");
	manager->port = port;
	manager->main_sd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	assert (manager->main_sd > 0);

	// prepare for bind
	memset ((void *)&manager->name, 0, sizeof (struct sockaddr_in));
	manager->name.sin_family = AF_INET;
	manager->name.sin_port = htons (port);
	manager->name.sin_addr.s_addr = htonl (INADDR_ANY);

	// bind
	zlog_debug (category, "Binding");
	int status = bind (manager->main_sd, (struct sockaddr *)&manager->name, sizeof (struct sockaddr_in));
	assert (status == 0);

	// listen
	zlog_debug (category, "Listening at %d.", port);
	status = listen (manager->main_sd, MAX_CONN);
	assert (status == 0);

	// epoll
	zlog_debug (category, "Setting up epoll.");
	manager->epoll_descriptor = epoll_create (MAX_EVENTS);
    assert (manager->epoll_descriptor >= 0);

    struct epoll_event event = {
    	.events = EPOLLIN,
    	.data.fd = manager->main_sd
    };
    status = epoll_ctl (manager->epoll_descriptor, EPOLL_CTL_ADD, manager->main_sd, &event);
    assert (status == 0);

    // sent buffer
    int n = 57654 * 100;
    setsockopt (manager->main_sd, SOL_SOCKET, SO_SNDBUF, &n, sizeof(n));


    // hook
    manager->run = _run_server;
    manager->count = 0;
	return 0;
}

int read_request (int sd, request_t *request) {
	assert (request != NULL);
	memset (request, 0, sizeof (request_t));

	zlog_category_t *category = zlog_get_category ("socket");

	int current;
	//int status = ioctl (sd, FIONREAD, &current);
	//assert (status == 0);

	// int i;
	// for (i = 0; i < 2; i++) {
	// 	ioctl (sd, FIONREAD, &current);
	// 	sleep (1);
	// }

	// if (current < sizeof (request_t) && current != 0) {
	// 	zlog_debug (category, "*** %d", current);
	// 	return current;
	// }
	
	int n = read (sd, (void *)request, sizeof (request_t));
	assert (n != -1);

	// client closed
	if (n == 0)
		return 0;

	zlog_debug (category, "##############   %d, %s", n, (char *)request);
	assert (n == sizeof (request_t));
	return n;
}

int read_response (int sd, response_t *response) {
	assert (response != NULL);
	memset (response, 0, sizeof (response_t));

	int n;
	n = recv (sd, (void *)response, sizeof (response_t), MSG_WAITALL);
	assert (n != -1);

	zlog_category_t *category = zlog_get_category ("client");
	//zlog_info (category, "READ %d", n);
	// server closed
	if (n == 0)
		return 0;

	assert (n == sizeof (response_t));

	// read movie
	if (response->len > 0) {
		void *movie = malloc (response->len);
		assert (movie != NULL);
		int status = recv (sd, movie, response->len, MSG_WAITALL);
		// while (status < response->len) {
		// 	status += read (sd, movie + status, response->len - status);
		// 	usleep (500);
		// }
		//zlog_info (category, "READ %d", status);

		response->data = movie;
	} else {

		response->len = 0;
		response->data = NULL;
	}
	
	return n;
}

int _run_client (socket_manager_t *manager) {
	assert (manager != NULL);
	assert (manager->client != NULL);

	zlog_category_t *category = zlog_get_category ("socket");
	zlog_info (category, "Running client.");


	while (manager->shutdown == false) {

		// malloc here, free at (*handle)
		response_t *response = (response_t *)malloc(sizeof (response_t));
		assert (response != NULL);

		int status = read_response (manager->main_sd, response);
		zlog_debug (category, "Response arrived.");

		// server closed
		if (status == 0) {
			zlog_info (category, "Connection closed.");
			manager->shutdown = true;
		} else {
			manager->handle (manager, manager->main_sd, (void *)response, sizeof (response_t), (void *)manager->client);
		}

	}

	zlog_info (category, "Destroying client socket manager.");	
	close (manager->main_sd);

}

int _run_server (socket_manager_t *manager) {
	assert (manager != NULL);
	assert (manager->server != NULL);

	zlog_category_t *category = zlog_get_category ("socket");
	zlog_info (category, "Running server.");

	struct epoll_event events[MAX_EVENTS];

	while (manager->shutdown == false) {

		int num = epoll_wait (manager->epoll_descriptor, events, MAX_EVENTS, 1000);
		int i;

		for (i = 0; i < num; i++) {
			// server
			if (events[i].data.fd == manager->main_sd) {

				zlog_debug (category, "Handling incoming connection.");

				// max connection reached
				if (manager->count >= MAX_CONN)
					assert (false);

				// accept
				int size = sizeof (struct sockaddr_in);
				int sd = accept (manager->main_sd, (struct sockaddr *)&manager->name, &size);
				manager->client_sds[manager->count] = sd;
				assert (sd >= 0);
				manager->count++;

				zlog_info (category, "New connection accepted at %d.", sd);

				// set to nonblock
				zlog_debug (category, "Set to nonblock.");
				//_set_nonblock (sd);

				// sent buffer
			    int n = 57654 * 100;
			    setsockopt (sd, SOL_SOCKET, SO_SNDBUF, &n, sizeof(n));


				// add to epoll
				zlog_debug (category, "Add to epoll.");
				struct epoll_event event = {
					.events = EPOLLIN,
					.data.fd = sd
				};

				int status = epoll_ctl (manager->epoll_descriptor, EPOLL_CTL_ADD, sd, &event);
				assert (status == 0);

			// client EPOLLIN
			} else if (events[i].events & EPOLLIN) {

				zlog_debug (category, "Request arrived at %d.", events[i].data.fd);

				// malloc here, free at (*handle)
				request_t *request = (request_t *)malloc(sizeof (request_t));
				assert (request != NULL);

				int sd = events[i].data.fd;
				int status = read_request (sd, request);

				// client closed
				if (status == 0) {
					close (sd);
					manager->count--;
				}
				// not enough
				else if (status < sizeof (request_t))
					continue;
				else {
					manager->handle (manager, sd, (void *)request, sizeof (request_t), (void *)manager->server);
				}

			}
		}
	}

	zlog_info (category, "Destroying server socket manager.");	
	close (manager->epoll_descriptor);
	close (manager->main_sd);
}

int destroy_socket_manager (socket_manager_t *manager) {
	manager->shutdown = true;
}