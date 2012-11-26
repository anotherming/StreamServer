#include "server.h"
#include "request.h"
#include <malloc.h>
#include <string.h>
#include "sortlist.h"
#include <time.h>
#include "movies.h"

// functions for socket manager
int _handle (socket_manager_t *manager, int sd, void *data, int len, void *args);
int _write (socket_manager_t *manager, int sd, void *data, int len, void *args);

// functions for server
int _run (server_t *server);
void* _do_run (void *server);

// handler
int _echo (socket_manager_t *manager, int sd, request_t *req, server_t *server);

// producer and consumer
void* _producer (void *args);
void* _consumer (void *args);

typedef struct {
	server_t *server;
	request_t *request;
} producer_arg_t;





void* _do_run (void *server) {
	assert (server != NULL);
	server_t *s = (server_t *)server;

	s->socket->run (s->socket);

	pthread_exit (NULL);
}

int _run (server_t *server) {
	assert (server != NULL);

    zlog_category_t *category = zlog_get_category("server");
	zlog_info (category, "Starting server loop.");

	// create thread for loop
	int status = pthread_create (&server->loop, NULL, _do_run, (void *)server);
	assert (status == 0);

	return 0;
}



int _write (socket_manager_t *manager, int sd, void *data, int len, void *args) {
	assert (manager != NULL);
	assert (sd > 0);
	assert (data != NULL);
	assert (len > 0);
	assert (args != NULL);

	// get log
    zlog_category_t *category = zlog_get_category("server");
    assert (category != NULL);

    // get server
	server_t *server = (server_t *)args;

	// get response
	assert (len == sizeof (response_t));
	response_t *response = (response_t *)data;

	// debug
	char buffer[256];
	encode_res (response, buffer);

	// do real write
	int n = 0;
	int status = write (sd, data, len);
	assert (status >= 0);
	n = status;

	while (n < len) {
		status = write (sd, data + status, len - status);
		assert (status > 0);
		n += status;
	}

	// write data
	if (response->len > 0) {
		status = write (sd, response->data, response->len);
		while (status < response->len) {
			status += write (sd, response->data + status, response->len - status);
		}

		free (response->data);
	}

	//zlog_info (category, "WRITE %d", status);
	

	// remember to free response
	//free (response);
}

int _echo (socket_manager_t *manager, int sd, request_t *request, server_t *server) {

    zlog_category_t *category = zlog_get_category("server");

    char buffer[256];
	encode_req (request, buffer);
	zlog_info (category, "REQUEST RECEIVED: %s", buffer);

}

bool _check_movie (request_t *request, server_t *server) {

    zlog_category_t *category = zlog_get_category("server");

    char filename[128];
    sprintf (filename, "%s%u.ppm", request->movie, request->frame_number);

    bool ret = server->loader.exists (&server->loader, filename);

 	return ret;
}

int _load_movie (request_t *request, server_t *server, void *buffer) {
	zlog_category_t *category = zlog_get_category("server");

	char filename[128];
	sprintf (filename, "%s%u.ppm", request->movie, request->frame_number);

	int fd = server->loader.open (&server->loader, filename);
	int len = server->loader.read (&server->loader, fd, 0, 100000, buffer);
}


int _handle (socket_manager_t *manager, int sd, void *data, int len, void *args) {
	assert (manager != NULL);
	assert (data != NULL);
	assert (len > 0);
	assert (args != NULL);

	// get log
    zlog_category_t *category = zlog_get_category("server");
    assert (category != NULL);

    // get server
	server_t *server = (server_t *)args;

	// get request
	assert (len == sizeof (request_t));
	request_t *request = (request_t *)data;

	// record the sd
	request->client_sd = sd;

	// debug
	char buffer[256];
	encode_req (request, buffer);
	zlog_debug (category, "Request being handled: %s", buffer);

	// do real handle
	_echo (manager, sd, request, server);

	if (_check_movie (request, server) == false) {
		response_t response;
		memcpy (&response.request, request, sizeof (request_t));
		response.worker_id = pthread_self ();
		response.data = malloc ((strlen ("NOT_EXIST") + 1) * sizeof (char));
		strcpy (response.data, "NOT_EXIST");
		response.len = strlen ("NOT_EXIST") + 1;


		server->socket->write (server->socket, response.request.client_sd, (void *)&response, sizeof (response_t), server);

		return;
	}


	// malloc here, free at _producer
	producer_arg_t *arg = (producer_arg_t *)malloc (sizeof (producer_arg_t));
	assert (arg != NULL);
	arg->server = server;
	arg->request = request;

	// remember to free request
	server->cluster.submit (&server->cluster, _producer, (void *)arg);
}

void* _producer (void *args) {
	assert (args != NULL);

	// get log
    zlog_category_t *category = zlog_get_category("server");
    zlog_debug (category, "Producer: worker loading data for a request.");

	producer_arg_t *p = (producer_arg_t *)args;

	
	buffer_entry_t entry;
	memcpy (&entry.response.request, p->request, sizeof (request_t));
	entry.response.worker_id = pthread_self ();
	if (strcmp (p->request->request, REQ_START) == 0) {
		
		int frame = 1;
		entry.response.request.frame_number = frame;
		while (_check_movie (&entry.response.request, p->server) == true) {
			void *movie = malloc (100000);
			int len = _load_movie (&entry.response.request, p->server, movie);

			entry.response.len = len;
			entry.response.data = movie;


			p->server->buffer.produce (&p->server->buffer, &entry);
			
			frame++;
			entry.response.request.frame_number = frame;
			usleep (10);
		}
	} else if (strcmp (p->request->request, REQ_SEEK) == 0) {

		int frame = p->request->frame_number;
		entry.response.request.frame_number = frame;
		while (_check_movie (&entry.response.request, p->server) == true) {
			void *movie = malloc (100000);
			int len = _load_movie (&entry.response.request, p->server, movie);

			entry.response.len = len;
			entry.response.data = movie;


			p->server->buffer.produce (&p->server->buffer, &entry);
			
			frame++;
			entry.response.request.frame_number = frame;
			usleep (10);
		}
	}

	free (p->request);
	free (args);
}

void* _consumer (void *args) {
	assert (args != NULL);

	// get log
    zlog_category_t *category = zlog_get_category("server");

	server_t *server = (server_t *)args;
	buffer_t *buffer = &server->buffer;

	sortlist_t *sortlist = NULL;
	buffer_entry_t *entry = NULL;
	int len = 0;
	time_t then = time (NULL);
	while (true) {

		entry = buffer->consume (buffer);
		assert (entry != NULL);

		sortlist = sortlist_insert (sortlist, entry);
		len++;

		time_t now = time (NULL);
		if (server->socket->shutdown == false && ((len == 10) || (len > 0 && ((now - then) >= 1)))) {
			zlog_debug (category, "Consumer: dispatch a response.");

			while (sortlist != NULL) {
				assert (&entry != NULL);
				sortlist = sortlist_remove (sortlist, &entry);

				char tmp[128];
				encode_res (&entry->response, tmp);
				zlog_info (category, "DISPATCH: %s", tmp);
				server->socket->write (server->socket, entry->response.request.client_sd, (void *)&entry->response, sizeof (response_t), server);
				free (entry);
				len--;
			}
			sortlist = NULL;
			then = time (NULL);
		}
	}

	//pthread_exit (NULL);
}

int init_server (server_t *server) {
	assert (server != NULL);

    zlog_category_t *category = zlog_get_category("server");
	zlog_info (category, "Initializing server.");

	// init thread cluster
	thread_pool_cluster_config_t config_cluster = {
		.pool_size = 10,
		.extend_threshold = 5,
		.cycle_of_seconds = 1
	};
	init_thread_pool_cluster (&server->cluster, &config_cluster);

	// init file loader 
	init_file_loader (&server->loader, 100, "../images/", filenames);

	// init ring buffer
	buffer_config_t config_buffer = {
		.buffer_size = 50,
		.consume_threshold = 1,
		.produce_threshold = 0
	};
	init_ring_buffer (&server->buffer, &config_buffer);

	zlog_debug (category, "Allocating socket manager.");
	server->socket = (socket_manager_t *)malloc (sizeof (socket_manager_t));
	assert (server->socket != NULL);

	// init socket
	init_socket_server (server->socket, 8888);

	// hook
	server->socket->handle = _handle;
	server->socket->server = server;
	server->socket->write = _write;
	server->run = _run;

	// consumer
	server->cluster.submit (&server->cluster, _consumer, (void *)server);

	return 0;
}

int destroy_server (server_t *server) {
	assert (server != NULL);

    zlog_category_t *category = zlog_get_category("server");
	zlog_info (category, "Destroying server.");

	
	// destroy loop thread
	zlog_debug (category, "Destroying server loop.");
	int status = pthread_cancel (server->loop);
	status = pthread_join (server->loop, NULL);
	assert (status == 0);

	zlog_debug (category, "Destroying file loader.");
	destroy_file_loader (&server->loader);


	zlog_debug (category, "Destroying server socket manager.");
	destroy_socket_manager (server->socket);
	free (server->socket);

	zlog_debug (category, "Destroying thread cluster.");
	destroy_thread_pool_cluster (&server->cluster);


	zlog_debug (category, "Destroying ring buffer.");
	destroy_ring_buffer (&server->buffer);




	return 0;
}