CC=gcc
PROJ=/home/num/xdmq
INC=-I${PROJ}/lib/raft/include -Iinclude
CFLAGS=-fPIC -O2 $(INC) -g # -fsanitize=address
LDFLAGS=${PROJ}/lib/libuv/.libs/libuv.a ${PROJ}/lib/raft/libraft.a ${PROJ}/lib/msgpack-c/libmsgpackc.a -pthread

OTMP = $(patsubst src/%.c,%.o,$(wildcard src/*.c))
OBJS = $(patsubst %,obj/%,$(OTMP))

# OBJS=main.o server.o client.o buffer.o raft_callbacks.o produce.o persist.o coredump.o consume.o

all: main

main: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -o $@ $^ -c

clean:
	rm -f obj/* main
