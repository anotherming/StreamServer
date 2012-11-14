#include "config.h"

typedef struct {
	int client_id;
	int priority;
	int client_seq_id;
	// char[REQUEST_NAME_SIZE] request;
	// int args_count;
	// int args_size;
	// void *args
} request_t;

int encode_req (request_t *request, char *buffer);
int decode_req (request_t *request, char *data, int len);