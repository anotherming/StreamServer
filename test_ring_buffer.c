#include "ring_buffer.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


int main () {
	buffer_config_t config = {.buffer_size = 20};

	buffer_class_t* buffer = get_global_ring_buffer (&config);

	int i;
	int pid;

	for (i = 0; i < 3; i++) {
		printf ("Forking from %d\n", getpid ());
		pid = fork ();
		if (pid == 0) {
			printf ("Forked %d\n", getpid ());
			int j;
			for (j = 0; j < 1000; j++) {
				buffer_entry_t entry = {
					.request = {
						.client_id = getpid (),
						.priority = 1,
						.client_seq_id = j
					},
					.response = {
						.worker_id = getpid (),
					},
					.global_seq_id = j,
				};
				buffer->produce (buffer, &entry);
			}
			exit (0);
		}
	}

	printf ("Forking from %d\n", getpid ());

	pid = fork ();
	if (pid == 0) {
		printf ("Forked %d\n", getpid ());
		int i;
		for (i = 0; i < 3000; i++) {
			buffer_entry_t* entry = buffer->consume (buffer);
		}
	}
}