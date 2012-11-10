
#include "ring_buffer.h"
#include <pthread.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "bool.h"
#include "zlog.h"

int _produce (buffer_class_t *buffer_class, buffer_entry_t *buffer_entry);
buffer_entry_t* _consume (buffer_class_t *buffer_class);
buffer_entry_t* _allocate_entry ();

int init_ring_buffer (buffer_class_t *buffer_class, buffer_config_t *config) {
	assert (buffer_class != NULL);
	assert (config != NULL);
	assert (config->buffer_size > 0);

	zlog_category_t *category = zlog_get_category ("ring_buffer");
	assert (category != NULL);

	zlog_debug (category, "Initializing ring buffer with size %d.", config->buffer_size);
	
	// malloc
	zlog_debug (category, "Allocating buffer memory.");
	buffer_t *buffer = &buffer_class->buffer;
	buffer->entries = (buffer_entry_t *)malloc(config->buffer_size * sizeof (buffer_entry_t));
	assert (buffer->entries != NULL);

	// reset buffer
	memset (buffer->entries, sizeof (buffer->entries), 0);

	// setup constants
	buffer->size = config->buffer_size;
	buffer->count = 0;
	buffer->consumer_index = 0;
	buffer->producer_index = 0;
	buffer_class->shutdown = false;

	// setup sync facilities
	zlog_debug (category, "Initializing mutex.");
	int status;
	status = pthread_mutex_init (&buffer->mutex, NULL);
	assert (status == 0);
	status = pthread_cond_init (&buffer->cond_not_full, NULL);
	assert (status == 0);
	status = pthread_cond_init (&buffer->cond_not_empty, NULL);
	assert (status == 0);

	// hooking methods
	buffer_class->produce = _produce;
	buffer_class->consume = _consume;
	return 0;
}

int destroy_ring_buffer (buffer_class_t *buffer_class) {
	assert (buffer_class != NULL);

	zlog_category_t *category = zlog_get_category ("ring_buffer");
	assert (category != NULL);

	buffer_class->shutdown = true;

	zlog_debug (category, "Destroying ring buffer.");

	


	int status;

	status = pthread_mutex_lock (&buffer_class->buffer.mutex);
	assert (status == 0);

	zlog_debug (category, "Destroying mutex.");

	pthread_cond_broadcast (&buffer_class->buffer.cond_not_full);
	status = pthread_cond_destroy (&buffer_class->buffer.cond_not_full);
	assert (status == 0);

	pthread_cond_broadcast (&buffer_class->buffer.cond_not_empty);
	status = pthread_cond_destroy (&buffer_class->buffer.cond_not_empty);
	assert (status == 0);

	status = pthread_mutex_unlock (&buffer_class->buffer.mutex);
	assert (status == 0);
	status = pthread_mutex_destroy (&buffer_class->buffer.mutex);
	while (status != 0) {
		status = pthread_mutex_destroy (&buffer_class->buffer.mutex);
	}
	assert (status == 0);
	

	zlog_debug (category, "Reclaiming memory.");
	assert (&buffer_class->buffer.entries != NULL);
	free (buffer_class->buffer.entries);

	return 0;
}

int _produce (buffer_class_t *buffer_class, buffer_entry_t *buffer_entry) {
	assert (buffer_class != NULL);
	assert (buffer_entry != NULL);

	zlog_category_t *category = zlog_get_category ("ring_buffer");
	assert (category != NULL);
	zlog_debug (category, "Producer of TID %u", pthread_self ());

	buffer_t *buffer = &buffer_class->buffer;

	// if shutdown
	if (buffer_class->shutdown == true) {
		zlog_debug (category, "Shutting down producer.");
		return 0;
	}

	// try to lock buffer
	zlog_debug (category, "Waiting for lock.");
	int status = pthread_mutex_lock (&buffer->mutex);
	assert (status == 0);
	zlog_debug (category, "Locked.");


	// wait for not full
	while (buffer->count == buffer->size) {  

		zlog_debug (category, "Buffer full. Waiting for consumers.");
	    status = pthread_cond_wait (&buffer->cond_not_full, &buffer->mutex);  
	    assert (status == 0);

	    // if shutdown
	    if (buffer_class->shutdown == true) {
	    	status = pthread_mutex_unlock (&buffer->mutex);
	    	assert (status == 0);
	    	return 0;
	    }
	}  	

	assert (buffer->count < buffer->size);

	// copy entry to buffer
	zlog_debug (category, "Writing to buffer.");
	memcpy (&(buffer->entries)[buffer->producer_index], buffer_entry, sizeof (buffer_entry_t));
	
	// update count
	zlog_debug (category, "Updating status.");
	buffer->count++;
	assert (buffer->count <= buffer->size);

	buffer->producer_index = (buffer->producer_index + 1) % buffer->size;
	zlog_debug (category, "Count %d, PI %d, CI %d", buffer->count, buffer->producer_index, buffer->consumer_index);

	// signal
	if (buffer->count == 1) {
		zlog_debug (category, "Signalling non-empty condition.");
	    status = pthread_cond_signal (&buffer->cond_not_empty);  
	    assert (status == 0);
	}  
	

	// unlock
	zlog_debug (category, "Finish writing. Unlock.");
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
	zlog_category_t *category = zlog_get_category ("ring_buffer");
	assert (category != NULL);

	zlog_debug (category, "Consumer of TID %u", pthread_self ());

	buffer_t *buffer = &buffer_class->buffer;


	// if shutdown
	if (buffer_class->shutdown == true) {
		zlog_debug (category, "Shutting down consumer.");
		return 0;
	}

	// try to lock
	zlog_debug (category, "Waiting for lock.");
	int status = pthread_mutex_lock (&buffer->mutex);      
	assert (status == 0);
	zlog_debug (category, "Locked.");
	//sleep (1);

	// wait for non-empty
	while (buffer->count == 0) {  

		zlog_debug (category, "Buffer empty. Waiting for producers.");
	    status = pthread_cond_wait (&buffer->cond_not_empty, &buffer->mutex);  
	    assert (status == 0);

	    // if shutdown
	    if (buffer_class->shutdown == true) {
	    	status = pthread_mutex_unlock (&buffer->mutex);
	    	assert (status == 0);
	    	return 0;
	    }
	} 

	assert (buffer->count > 0);

	// allocate entry
	zlog_debug (category, "Reading from buffer.");
	buffer_entry_t *entry = _allocate_entry ();
	memcpy (entry, &(buffer->entries)[buffer->consumer_index], sizeof (buffer_entry_t));

	// update
	zlog_debug (category, "Updating status.");
	buffer->count--;
	assert (buffer->count >= 0);

	buffer->consumer_index = (buffer->consumer_index + 1) % buffer->size;
	zlog_debug (category, "Count %d, PI %d, CI %d", buffer->count, buffer->producer_index, buffer->consumer_index);

	// non full
	if (buffer->count == buffer->size - 1) {  
		zlog_debug (category, "Signalling non-full condition.");
	    status = pthread_cond_signal (&buffer->cond_not_full);  
	    assert (status == 0);
	}  

	// unlock
	zlog_debug (category, "Finish reading. Unlock.");
	status = pthread_mutex_unlock (&buffer->mutex);  
	assert (status == 0);

	

	return entry;
}






