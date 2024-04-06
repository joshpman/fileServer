CC=gcc
CFLAGS=-Wall -pedantic -g

all: client server
client: client.o
	$(CC) $(CFLAGS) -o client client.o
server: server.o
	$(CC) $(CFLAGS) -o server server.o

.PHONY: clean
clean:
	rm *.o client server udgram mynewaddr
	