all: gradebookadd gradebookdisplay setup

LFLAGS=-lcrypto -lm

gradebookadd: gradebookadd.o data.o
	$(CC) $(DEBUG) -o gradebookadd gradebookadd.o data.o $(LFLAGS)

gradebookdisplay: gradebookdisplay.o data.o
	$(CC) $(DEBUG) -o gradebookdisplay gradebookdisplay.o data.o $(LFLAGS)

setup: setup.o data.o
	$(CC) $(DEBUG) -o setup setup.o data.o $(LFLAGS)
	rm *.o

gradebookadd.o: gradebookadd.c
	$(CC) $(DEBUG) -c -o gradebookadd.o gradebookadd.c

gradebookdisplay.o: gradebookdisplay.c
	$(CC) $(DEBUG) -c -o gradebookdisplay.o gradebookdisplay.c

setup.o: setup.c
	$(CC) $(DEBUG) -c -o setup.o setup.c

data.o: data.c
	$(CC) $(DEBUG) -c -o data.o data.c

clean:
	rm -f *.o
	rm -rf gradebookadd gradebookdisplay setup

debug: DEBUG = -DDEBUG
debug: clean all
