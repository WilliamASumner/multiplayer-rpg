

default: rpg_client rpg_server
debug: rpg_client_debug rpg_server_debug

multi: rpg_client rpg_server_multi

multidebug: rpg_client_debug rpg_server_multi_debug

rpg_client: rpg_client.c common.c
	gcc -o $@ $< common.c

rpg_client_debug: rpg_client.c common.c
	gcc -o $@ $< common.c -g

rpg_server_multi: rpg_server.c common.c
	gcc -o $@ $< -pthread -D_REENTRANT common.c

rpg_server_multi_debug: rpg_server.c common.c
	gcc -o $@ $< -pthread -D_REENTRANT common.c -g

rpg_server: rpg_server.c common.c
	gcc -o $@ $< common.c

rpg_server_debug: rpg_server.c common.c
	gcc -o $@ $< common.c -g

clean:
	rm -r rpg_client rpg_server rpg_server_multi *_debug *.dSYM
