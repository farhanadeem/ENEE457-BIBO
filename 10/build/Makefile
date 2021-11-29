

all: setup gradebookadd gradebookdisplay
	

setup: setup.o utils.o data.o db.o
	gcc -Wall -g setup.o utils.o data.o db.o -o setup -lcrypto

gradebookadd: add.o utils.o data.o db.o
	gcc -Wall -g add.o utils.o data.o db.o -o gradebookadd -lcrypto

gradebookdisplay: display.o utils.o data.o db.o print.o
	gcc -Wall -g display.o utils.o data.o db.o print.o -o gradebookdisplay -lcrypto

setup.o:
	gcc -Wall -g -c ../src/setup.c -o setup.o

utils.o:
	gcc -Wall -g -c ../src/utils.c -o utils.o

data.o:
	gcc -Wall -g -c ../src/data.c -o data.o

db.o:
	gcc -Wall -g -c ../src/db.c -o db.o

add.o:
	gcc -Wall -g -c ../src/add.c -o add.o

display.o:
	gcc -Wall -g -c ../src/display.c -o display.o

print.o:
	gcc -Wall -g -c ../src/print.c -o print.o

clean:
	rm *.o
	rm setup
	rm gradebookadd
	rm gradebookdisplay

