CC=gcc
CFLAGS=-g -Wall -Iinclude

all: clean blog run

blog: build/main.o build/md4c.o build/md4c-html.o build/entity.o
	$(CC) $(CFLAGS) build/main.o build/md4c.o build/md4c-html.o build/entity.o -o build/blog

build/main.o:
	$(CC) $(CFLAGS) -c src/main.c -o build/main.o

build/md4c.o:
	$(CC) $(CFLAGS) -c src/md4c.c -o build/md4c.o

build/md4c-html.o:
	$(CC) $(CFLAGS) -c src/md4c-html.c -o build/md4c-html.o

build/entity.o:
	$(CC) $(CFLAGS) -c src/entity.c -o build/entity.o

run:
	./build/blog

clean:
	rm build/*
