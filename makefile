CC = gcc
CFLAGS = -Wall
LIBS = -lpthread

c: c.c
	$(CC) -o c c.c $(LIBS)
clean:
	rm -f c