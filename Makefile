CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -g3

all: server client

server: server.o common.o
	$(CC) $(LDFLAGS) $^ -o $@

client: client.o common.o
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o server client
