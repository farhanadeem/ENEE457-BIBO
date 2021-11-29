CFLAGS=-g
LFLAGS=-lssl -lcrypto

all: setup gradebookadd gradebookdisplay

setup: setup.o data.o
	$(CC) $(CFLAGS) -o setup setup.o data.o $(LFLAGS)
	
gradebookadd: gradebookadd.o data.o
	$(CC) -g -o gradebookadd gradebookadd.o data.o $(LFLAGS)
	
gradebookdisplay: gradebookdisplay.o data.o
	$(CC) -g -o gradebookdisplay gradebookdisplay.o data.o $(LFLAGS)
	
gradebookdisplay.o: gradebookdisplay.c
	$(CC) $(CFLAGS) -c -o gradebookdisplay.o gradebookdisplay.c

gradebookadd.o: gradebookadd.c
	$(CC) $(CFLAGS) -c -o gradebookadd.o gradebookadd.c

setup.o: setup.c
	$(CC) $(CFLAGS) -c -o setup.o setup.c

data.o: data.c
	$(CC) $(CFLAGS) -c -o data.o data.c

clean:
	rm -f *.o
	rm -rf gradebookadd gradebookdisplay setup
