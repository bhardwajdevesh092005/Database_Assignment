# Makefile for the Slotted Page (SP) layer

# Compiler and flags
# Use the same C89 standard for compatibility with the rest of the project.
# Add include paths for the current directory and the pflayer.
CC = gcc -std=c89 -Wno-implicit-function-declaration
CFLAGS = -g -Wall -I. -I../pflayer

# Object files for the SP layer
# For now, this is just sp.c, but can be extended.
OBJS = sp.o

# PFLayer object files needed for linking the test executable
PF_OBJS = ../pflayer/pf.o ../pflayer/buf.o ../pflayer/hash.o

# Default target builds the test program
TARGETS = test_sp test_stats test_linear_scan

all: $(TARGETS)

# Rule to build the test executable
# It depends on the test object file, the SP layer objects, and the PF layer objects.
test_sp: test_sp.o sp.o
	$(CC) $(CFLAGS) -o test_sp test_sp.o sp.o ../pflayer/pf.o ../pflayer/buf.o ../pflayer/hash.o -lm

# Rule to build the stats test executable
test_stats: test_stats.o sp.o
	$(CC) $(CFLAGS) -o test_stats test_stats.o sp.o ../pflayer/pf.o ../pflayer/buf.o ../pflayer/hash.o -lm

# Rule to build the linear scan test executable
test_linear_scan: test_linear_scan.o sp.o
	$(CC) $(CFLAGS) -o test_linear_scan test_linear_scan.o sp.o ../pflayer/pf.o ../pflayer/buf.o ../pflayer/hash.o -lm

# Rule to compile the SP layer source file
sp.o: sp.c sp.h sptype.h
	$(CC) $(CFLAGS) -c sp.c

# Rule to compile the test driver (test_sp.c will be created later)
test_sp.o: test_sp.c sp.h sptype.h
	$(CC) $(CFLAGS) -c test_sp.c

# Rule to compile the stats test driver
test_stats.o: test_stats.c sp.h ../pflayer/pf.h
	$(CC) $(CFLAGS) -c test_stats.c

# Rule to compile the linear scan test driver
test_linear_scan.o: test_linear_scan.c sp.h ../pflayer/pf.h
	$(CC) $(CFLAGS) -c test_linear_scan.c

# Clean up generated files
clean:
	rm -f test_sp test_stats test_linear_scan *.o *~

.PHONY: all clean
