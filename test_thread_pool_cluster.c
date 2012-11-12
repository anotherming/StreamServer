#include "thread_pool_cluster.h"
#include <assert.h>
#include "zlog.h"

zlog_category_t *category;

void *work (void *args) {
	assert (args != NULL);

	thread_pool_cluster_t *cluster = (thread_pool_cluster_t *)args;
	int sum = 0, i;
	for (i = 0; i < MAXPOOL; i++)
		if (cluster->masks[i] == true)
			sum++;

	//zlog_debug (category, "COUNT: %d", sum);
	sleep (1);
	return NULL;
}

int main () {
	int state = zlog_init("../zlog.conf");
	assert (state == 0);
    category = zlog_get_category("default");
	assert (category != NULL);
    zlog_debug (category, "Start!");


	thread_pool_cluster_t cluster;
	thread_pool_cluster_config_t config = {
		.pool_size = 5,
		.extend_threshold = 5,
		.cycle_of_seconds = 1
	};

	init_thread_pool_cluster (&cluster, &config);

	int i = 0;
	for (i = 0; i < 10; i++) {
		cluster.submit (&cluster, work, &cluster);
	}

	sleep (5);

	for (i = 0; i < 100; i++) {
		cluster.submit (&cluster, work, &cluster);
	}

sleep (5);

	for (i = 0; i < 100; i++) {
		cluster.submit (&cluster, work, &cluster);
	}

	sleep (30);

	destroy_thread_pool_cluster (&cluster);

	zlog_fini();
}