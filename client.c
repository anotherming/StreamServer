#include "client.h"
#include "request.h"
#include "response.h"
#include <pthread.h>
#include "zlog.h"
#include <assert.h>
#include <malloc.h>
#include <string.h>

// functions for socket manager
int _handle_client (socket_manager_t *manager, int sd, void *data, int len, void *args);
int _write_client (socket_manager_t *manager, int sd, void *data, int len, void *args);

// functions for client
int _run_c (client_t *client);
void* _do_run_c (void *client);

// functions for client
int _start_movie (client_t *client, char *name, bool repeat);
int _stop (client_t *client, char *name);
int _seek (client_t *client, char *name, int frame);

// inner functions
int _send (client_t *client, request_t *request);



int _send (client_t *client, request_t *request) {
	assert (client != NULL);
	assert (request != NULL);

	request->client_sd = 0;

	zlog_category_t *category = zlog_get_category ("client");
	char tmp[128];
	encode_req (request, tmp);
	zlog_info (category, "REQUEST SENT: %s", tmp);


	client->socket->write (client->socket, client->socket->main_sd, (void *)request, sizeof (request_t), (void *)client);

	return 0;
}


int _start_movie (struct client_t *client, char *name, bool repeat) {
	assert (client != NULL);
	assert (name != NULL);

	zlog_category_t *category = zlog_get_category ("client");
	zlog_debug (category, "Request: start movie %s", name);

	request_t request;

	request.client_id = client->client_id;
	request.priority = client->priority;
	strcpy (request.request, REQ_START);
	strcpy (request.movie, name);
	request.repeat = repeat;
	request.frame_number = 1;

	int i = 1;
	client->play = true;

	_send (client, &request);

}

int _stop (struct client_t *client, char *name) {
	assert (client != NULL);
	assert (name != NULL);

	zlog_category_t *category = zlog_get_category ("client");
	zlog_debug (category, "Request: stop movie");

	request_t request;

	request.client_id = client->client_id;
	request.priority = client->priority;

	_send (client, &request);
}

int _seek (struct client_t *client, char *name, int frame) {
	assert (client != NULL);
	assert (name != NULL);

	zlog_category_t *category = zlog_get_category ("client");
	zlog_debug (category, "Request: seek movie %s", name);

	request_t request;

	request.client_id = client->client_id;
	request.priority = client->priority;
	strcpy (request.request, REQ_SEEK);
	strcpy (request.movie, name);
	request.repeat = false;
	request.frame_number = frame;

	int i = 1;
	client->play = true;

	_send (client, &request);
}


void* _do_run_c (void *client) {
	assert (client != NULL);
	client_t *c = (client_t *)client;

	c->socket->run (c->socket);

	pthread_exit (NULL);

}

int _run_c (client_t *client) {
	assert (client != NULL);

	zlog_category_t *category = zlog_get_category ("client");
	zlog_info (category, "Starting client loop.");

	// create thread for loop
	int status = pthread_create (&client->loop, NULL, _do_run_c, (void *)client);
	assert (status == 0);

	return 0;
}

int _write_client (socket_manager_t *manager, int sd, void *data, int len, void *args) {
	assert (manager != NULL);
	assert (data != NULL);
	assert (len > 0);
	assert (args != NULL);

	// get log
    zlog_category_t *category = zlog_get_category("client");

    // get client
	client_t *client = (client_t *)args;

	// get request
	assert (len == sizeof (request_t));
	request_t *request = (request_t *)data;

	// debug
	char buffer[256];
	encode_req (request, buffer);
	zlog_debug (category, "Request being sent: %s", buffer);

	// do real write, blocked
	int status = write (sd, data, len);
	assert (status == len);
}


int _handle_client (socket_manager_t *manager, int sd, void *data, int len, void *args) {
	assert (manager != NULL);
	assert (data != NULL);
	assert (len > 0);
	assert (args != NULL);

	// get log
    zlog_category_t *category = zlog_get_category("client");
    assert (category != NULL);

    // get client
	client_t *client = (client_t *)args;

	// get response
	assert (len == sizeof (response_t));
	response_t *response = (response_t *)data;

	// debug
	char buffer[256];
	encode_res (response, buffer);
	zlog_debug (category, "Response being handled: %s", buffer);

	// do real handle
	zlog_info (category, "RESPONSE RECEIVED: %s", buffer);
	if (response->len > 42)
		((char *)response->data)[41] = '\0';
	zlog_info (category, "%s", response->data);

	// NOT EXIST
	//if (response->len > 0 && strcmp (response->data, "NOT_EXIST") == 0)
	//	client->play = false;

	free (response->data);
	// remember to free response
	free (data);
}

int init_client (client_t *client, int priority) {
	assert (client != NULL);
	assert (priority >= 1);

	zlog_category_t *category = zlog_get_category ("client");
	zlog_info (category, "Initializing client.");

	zlog_debug (category, "Creating socket descriptor.");
	client->socket = (socket_manager_t *)malloc (sizeof (socket_manager_t));
	assert (client->socket != NULL);

	init_socket_client (client->socket, 8888, "127.0.0.1");

	// hook
	client->client_id = pthread_self ();
	client->priority = priority;
	client->socket->client = client;
	client->socket->handle = _handle_client;
	client->socket->write = _write_client;
	client->run = _run_c;
	client->start = _start_movie;
	client->seek = _seek;
	client->play = false;

	return 0;
}

int destroy_client (client_t *client) {
	assert (client != NULL);

	zlog_category_t *category = zlog_get_category ("client");
	zlog_info (category, "Destroying client.");

	// destroy loop thread
	zlog_debug (category, "Destroying client loop.");
	int status = pthread_cancel (client->loop);
	status = pthread_join (client->loop, NULL);
	assert (status == 0);

	zlog_debug (category, "Destroying client socket manager.");
	destroy_socket_manager (client->socket);
	free (client->socket);

	return 0;
}