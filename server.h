
#include "thread_pool_cluster.h"
#include "ring_buffer.h"
#include "file_loader.h"
#include <assert.h>


typedef struct {
	thread_pool_cluster_t cluster;
	buffer_t buffer;
	file_loader_t loader;


} server_t;




