CC=gcc
CFLAGS=-Wall -pedantic -g

all: client server wc
client: client.o
	$(CC) $(CFLAGS) -o client client.o
server: server.o
	$(CC) $(CFLAGS) -o server server.o
wc: wc.o
	$(CC) $(CFLAGS) -o wc wc.o
.PHONY: clean
clean:
	rm *.o client server wc
	