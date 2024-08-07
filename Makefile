all:
	gcc -o sqlite sqlite.c -lsqlite3 -L/usr/lib/x86_64-linux-gnu -I/usr/include
	gcc -o server server.c -lpthread -lsqlite3 -L/usr/lib/x86_64-linux-gnu -I/usr/include
	gcc -o client client.c -lsqlite3 -L/usr/lib/x86_64-linux-gnu -I/usr/include

.PHONY:clean
clean:
	rm server client sqlite