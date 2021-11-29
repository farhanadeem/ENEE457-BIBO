all: gradebookadd gradebookdisplay setup

CFLAGS=-g 
LFLAGS=-lssl -lcrypto 

gradebookadd: gradebookadd_skel.o enc_and_dec.o
	$(CC) -g -o gradebookadd gradebookadd_skel.o enc_and_dec.o $(LFLAGS)

gradebookdisplay: gradebookdisplay_skel.o enc_and_dec.o
	$(CC) $(CFLAGS) -o gradebookdisplay gradebookdisplay_skel.o enc_and_dec.o $(LFLAGS)

setup: setup_skel.o enc_and_dec.o
	$(CC) $(CFLAGS) -o setup setup_skel.o enc_and_dec.o $(LFLAGS)




gradebookadd_skel.o: gradebookadd_skel.c 
	$(CC) $(CFLAGS) -c -o gradebookadd_skel.o gradebookadd_skel.c

gradebookdisplay_skel.o: gradebookdisplay_skel.c
	$(CC) $(CFLAGS) -c -o gradebookdisplay_skel.o gradebookdisplay_skel.c

setup.o: setup_skel.c
	$(CC) $(CFLAGS) -c -o setup_skel.o setup_skel.c


enc_and_dec.o: enc_and_dec.c 
	$(CC) $(CFLAGS) -c -o enc_and_dec.o enc_and_dec.c



clean:
	rm -f *.o
	rm -rf gradebookadd_skel gradebookdisplay_skel setup_skel
