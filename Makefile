CC=gcc
INC=-I/home/num/raft/include
CFLAGS=-fPIC -O0 $(INC) -g
LDFLAGS=-luv -L/home/num/raft -lraft -lmsgpackc

OBJS=main.o server.o client.o buffer.o raft_callbacks.o

all: main

main: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm *.o main
