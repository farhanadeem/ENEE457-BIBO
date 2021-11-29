.PHONY : prereqs
all: prereqs all2

prereqs:
	cd sql && ./configure
	cd sql && make
	cd sql && sudo make install

all2: gradebookadd gradebookdisplay setup

CFLAGS=-g 
LFLAGS=-lssl -lcrypto -lsqlite3

gradebookadd: gradebookadd.o functions.o
	$(CC) -g -o gradebookadd gradebookadd.o functions.o $(LFLAGS)

gradebookdisplay: gradebookdisplay.o functions.o
	$(CC) $(CFLAGS) -o gradebookdisplay gradebookdisplay.o functions.o $(LFLAGS)

setup: setup.o functions.o
	$(CC) $(CFLAGS) -o setup setup.o functions.o $(LFLAGS)

gradebookadd.o: gradebookadd.c
	$(CC) $(CFLAGS) -c -o gradebookadd.o gradebookadd.c

gradebookdisplay.o: gradebookdisplay.c
	$(CC) $(CFLAGS) -c -o gradebookdisplay.o gradebookdisplay.c

setup.o: setup.c
	$(CC) $(CFLAGS) -c -o setup.o setup.c

functions.o: functions.c
	$(CC) $(CFLAGS) -c -o functions.o functions.c
clean:
	rm -f *.o
	rm -rf gradebookadd gradebookdisplay setup
