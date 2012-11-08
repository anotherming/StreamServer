
#include "ring_buffer.h"
#include <pthread.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "bool.h"

int _initialize (buffer_class_t *buffer_class, buffer_config_t *config);
int _destroy (buffer_class_t *buffer_class);
int _produce (buffer_class_t *buffer_class, buffer_entry_t *buffer_entry);
buffer_entry_t* _consume (buffer_class_t *buffer_class);
buffer_entry_t* _allocate_entry ();

int _initialize (buffer_class_t *buffer_class, buffer_config_t *config) {
	assert (buffer_class != NULL);
	assert (config != NULL);
	assert (buffer_class->is_initialized == false);

	buffer_class->buffer.buffer = (buffer_entry_t *)malloc(config->buffer_size * sizeof (buffer_entry_t));
	memset (&buffer_class->buffer.buffer, sizeof (buffer_class->buffer.buffer), 0);
	buffer_class->buffer.size = config->buffer_size;
	buffer_class->buffer.count = 0;
	buffer_class->buffer.consumer_index = 0;
	buffer_class->buffer.producer_index = 0;

	int status;
	status = pthread_mutex_init (&buffer_class->buffer.mutex, NULL);
	assert (status == 0);
	status = pthread_cond_init (&buffer_class->buffer.cond_not_full, NULL);
	assert (status == 0);
	status = pthread_cond_init (&buffer_class->buffer.cond_not_empty, NULL);
	assert (status == 0);

	buffer_class->is_initialized = true;

	return 0;
}

int _destroy (buffer_class_t *buffer_class) {
	assert (buffer_class != NULL);
	free (&buffer_class->buffer);

	int status;
	status = pthread_mutex_destroy (&buffer_class->buffer.mutex);
	assert (status == 0);
	status = pthread_cond_destroy (&buffer_class->buffer.cond_not_full);
	assert (status == 0);
	status = pthread_cond_destroy (&buffer_class->buffer.cond_not_empty);	
	assert (status == 0);

	return 0;
}

int _produce (buffer_class_t *buffer_class, buffer_entry_t *buffer_entry) {

	assert (buffer_class != NULL);
	assert (buffer_entry != NULL);

	buffer_t *buffer = &(buffer_class->buffer);

	int status = pthread_mutex_lock (&buffer->mutex);      
	assert (status == 0);

	if (buffer->count == buffer->size) {  
	    status = pthread_cond_wait (&buffer->cond_not_full, &buffer->mutex);  
	    assert (status == 0);
	}  	

	printf ("Producing %d from %d at %d\n", buffer_entry->request.client_seq_id, getpid (), buffer->producer_index);



	memcpy (&(buffer->buffer)[buffer->producer_index], buffer_entry, sizeof (buffer_entry_t));
	buffer->count++;
	buffer->producer_index = (buffer->producer_index + 1) % buffer->size;

	if (buffer->count == 1) {  
	    status = pthread_cond_signal (&buffer->cond_not_empty);  
	    assert (status == 0);
	}  

	status = pthread_mutex_unlock (&buffer->mutex);  
	assert (status == 0);
	return 0;
}

buffer_entry_t* _allocate_entry () {
	buffer_entry_t* ret = (buffer_entry_t *)malloc(sizeof (buffer_entry_t));
	assert (ret != NULL);
	return ret;
}

buffer_entry_t* _consume (buffer_class_t *buffer_class) {
	assert (buffer_class != NULL);

	buffer_t *buffer = &(buffer_class->buffer);

	int status = pthread_mutex_lock (&buffer->mutex);      
	assert (status == 0);

	if (buffer->count == 0) {  
	    status = pthread_cond_wait (&buffer->cond_not_empty, &buffer->mutex);  
	    assert (status == 0);
	}  

	buffer_entry_t *entry = buffer_class->allocate_entry ();

	memcpy (entry, &(buffer->buffer)[buffer->consumer_index], sizeof (buffer_entry_t));

	printf ("Consuming %d", entry->request.client_seq_id);



	buffer->count--;
	buffer->consumer_index = (buffer->consumer_index + 1) % buffer->size;

	if (buffer->count == buffer->size - 1) {  
	    status = pthread_cond_signal (&buffer->cond_not_full);  
	    assert (status == 0);
	}  

	status = pthread_mutex_unlock (&buffer->mutex);  
	assert (status == 0);
return
	 entry;
}

buffer_class_t _ring_buffer = {
	.is_initialized = false,
	.initialize = _initialize,
	.destroy = _destroy,
	.produce = _produce,
	.consume = _consume,
	.allocate_entry = _allocate_entry
};


buffer_class_t* get_global_ring_buffer (buffer_config_t *config) {
	if (_ring_buffer.is_initialized == true)
		return &_ring_buffer;

	_ring_buffer.initialize (&_ring_buffer, config);
	return &_ring_buffer;
}



