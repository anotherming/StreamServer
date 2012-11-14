

#include "config.h"

typedef struct {
	int worker_id;
} response_t;


int encode_res (response_t *response, char *buffer);
int decode_res (response_t *response, char *data, int len);
