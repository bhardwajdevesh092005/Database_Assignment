# Makefile for Bulk-Load Layer

CC = gcc -std=c89 -Wno-implicit-function-declaration
CFLAGS = -g -Wall -I. -I../pflayer

# Object files
OBJS = bl.o blbulk.o blsearch.o blscan.o blprint.o

# PF layer objects
PF_OBJS = ../pflayer/pf.o ../pflayer/buf.o ../pflayer/hash.o

# Default target
all: testbl

# Test program
testbl: testbl.o $(OBJS) $(PF_OBJS)
	$(CC) $(CFLAGS) -o testbl testbl.o $(OBJS) $(PF_OBJS) -lm

# Compile BL layer files
bl.o: bl.c bl.h blinternal.h
	$(CC) $(CFLAGS) -c bl.c

blbulk.o: blbulk.c bl.h blinternal.h
	$(CC) $(CFLAGS) -c blbulk.c

blsearch.o: blsearch.c bl.h blinternal.h
	$(CC) $(CFLAGS) -c blsearch.c

blscan.o: blscan.c bl.h blinternal.h
	$(CC) $(CFLAGS) -c blscan.c

blprint.o: blprint.c bl.h blinternal.h
	$(CC) $(CFLAGS) -c blprint.c

testbl.o: testbl.c bl.h
	$(CC) $(CFLAGS) -c testbl.c

clean:
	rm -f testbl *.o *~ *.db

.PHONY: all clean