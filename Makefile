CC=gcc
CFLAGS=-Wall -Wextra -O3 -pthread
CFILES=$(shell ls ./src/*.c)
PROGS=$(CFILES:./src/%.c=%)

client: ./src/client.c
	mkdir -p ./ClientFiles
	$(CC) $(CFLAGS) -o $@ $^
	@echo "...Folder Created ...Building Done"

server: ./src/server.c
	mkdir -p ./ServerFiles
	$(CC) $(CFLAGS) -o $@ $^
	@echo "...Folder Created ...Building Done"

clean:
	rm -r -f ./ClientFiles & rm -r -f ./ServerFiles & rm -f $(PROGS) *.o
	@echo "...Clean Done"
