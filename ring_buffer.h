
#include "config.h"
#include "request.h"
#include "response.h"
#include "bool.h"
#include <pthread.h>

typedef struct {
	request_t request;
	response_t response;
	int global_seq_id;
} buffer_entry_t;

typedef struct {
	buffer_entry_t *buffer;
	pthread_mutex_t mutex;
	pthread_cond_t cond_not_full;
	pthread_cond_t cond_not_empty;

	int size;
	int count;
	int consumer_index;
	int producer_index;

} buffer_t;

typedef struct {
	int buffer_size;
} buffer_config_t;

typedef struct _buffer_class_t {
	bool is_initialized;
	int (*initialize) (struct _buffer_class_t *buffer_class, buffer_config_t *config);
	int (*destroy) (struct _buffer_class_t *buffer_class);
	int (*produce) (struct _buffer_class_t *buffer_class, buffer_entry_t *buffer_entry);
	buffer_entry_t* (*consume) (struct _buffer_class_t *buffer_class);
	buffer_entry_t* (*allocate_entry) ();
	buffer_t buffer;
} buffer_class_t;


buffer_class_t* get_global_ring_buffer (buffer_config_t *config);