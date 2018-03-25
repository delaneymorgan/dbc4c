# Makefile for dbctest

depend:
	$(CC) -I./ -M *.c > .depend

.c.o:
	$(CC) -c $(CFLAGS) $*.c

all: dbctest

dbctest:  dbctest.o
	$(CC) $(CFLAGS) dbctest.o -o dbctest

preprocess:
	$(CC) -I./ dbctest.c -E -o dbctest.i

clean:
	rm -f *.o
	rm -f *~
	rm -f dbctest dbctest.i
	
-include .depend
