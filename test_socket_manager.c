#include "server.h"
#include "client.h"
#include "socket_manager.h"

zlog_category_t *category;
server_t server;
client_t client[10];

int main () {
	int state = zlog_init("../zlog.conf");
	assert (state == 0);
    category = zlog_get_category("default");
	assert (category != NULL);
    zlog_debug (category, "Start!");

    init_server (&server);
    server.run (&server);

    int i, j;
    for (i = 0; i < 10; i++) {
		init_client (&client[i], i+1);
    	client[i].run (&client[i]);
    }

    for (i = 0; i < 10; i++)
        //for (j = 0; j < 100; j++)
           client[i].seek (&client[i], "sw", 50);


    sleep (30);

    for (i = 0; i < 10; i++) {
    	destroy_client (&client[i]);
    }

    destroy_server (&server);

    zlog_fini ();
}