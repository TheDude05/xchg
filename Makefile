PROG=xchg
CC=gcc
CFLAGS=-g -Wall
LDFLAGS=

$(PROG): $(PROG).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROG) $(PROG).c

clean:
	rm -f *.o
	rm -f $(PROG)
