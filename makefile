# Top-level Makefile to build the final indexing test

CC = gcc -std=c89 -Wno-implicit-function-declaration
CFLAGS = -g -Wall

# Include paths for all layers
INCLUDES = -I./pflayer -I./splayer -I./amlayer -I./bulklayer

# Source files from all layers
PF_SRCS = pflayer/pf.c pflayer/buf.c pflayer/hash.c
SP_SRCS = splayer/sp.c
AM_SRCS = amlayer/am.c amlayer/aminsert.c amlayer/amscan.c amlayer/amsearch.c \
          amlayer/amstack.c amlayer/amglobals.c amlayer/amfns.c amlayer/amprint.c amlayer/misc.c
BL_SRCS = bulklayer/bl.c bulklayer/blbulk.c bulklayer/blsearch.c bulklayer/blscan.c bulklayer/blprint.c

# Object files to be created
OBJS = $(PF_SRCS:.c=.o) $(SP_SRCS:.c=.o) $(AM_SRCS:.c=.o) $(BL_SRCS:.c=.o) test_indexing.o

# Default target
all: test_indexing

# Rule to link the final executable
test_indexing: $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o test_indexing $(OBJS) -lm

# --- CORRECTED BUILD RULES ---
# Be explicit about how to build each object file.

# Application object
test_indexing.o: test_indexing.c
	$(CC) $(CFLAGS) $(INCLUDES) -c test_indexing.c

# PF Layer objects
pflayer/pf.o: pflayer/pf.c
	$(CC) $(CFLAGS) $(INCLUDES) -c pflayer/pf.c -o pflayer/pf.o
pflayer/buf.o: pflayer/buf.c
	$(CC) $(CFLAGS) $(INCLUDES) -c pflayer/buf.c -o pflayer/buf.o
pflayer/hash.o: pflayer/hash.c
	$(CC) $(CFLAGS) $(INCLUDES) -c pflayer/hash.c -o pflayer/hash.o

# SP Layer object
splayer/sp.o: splayer/sp.c
	$(CC) $(CFLAGS) $(INCLUDES) -c splayer/sp.c -o splayer/sp.o

# AM Layer objects
amlayer/am.o: amlayer/am.c
	$(CC) $(CFLAGS) $(INCLUDES) -c amlayer/am.c -o amlayer/am.o
amlayer/aminsert.o: amlayer/aminsert.c
	$(CC) $(CFLAGS) $(INCLUDES) -c amlayer/aminsert.c -o amlayer/aminsert.o
amlayer/amscan.o: amlayer/amscan.c
	$(CC) $(CFLAGS) $(INCLUDES) -c amlayer/amscan.c -o amlayer/amscan.o
amlayer/amsearch.o: amlayer/amsearch.c
	$(CC) $(CFLAGS) $(INCLUDES) -c amlayer/amsearch.c -o amlayer/amsearch.o
amlayer/amstack.o: amlayer/amstack.c
	$(CC) $(CFLAGS) $(INCLUDES) -c amlayer/amstack.c -o amlayer/amstack.o
amlayer/amglobals.o: amlayer/amglobals.c
	$(CC) $(CFLAGS) $(INCLUDES) -c amlayer/amglobals.c -o amlayer/amglobals.o
amlayer/amfns.o: amlayer/amfns.c
	$(CC) $(CFLAGS) $(INCLUDES) -c amlayer/amfns.c -o amlayer/amfns.o
amlayer/amprint.o: amlayer/amprint.c
	$(CC) $(CFLAGS) $(INCLUDES) -c amlayer/amprint.c -o amlayer/amprint.o
amlayer/misc.o: amlayer/misc.c
	$(CC) $(CFLAGS) $(INCLUDES) -c amlayer/misc.c -o amlayer/misc.o

# BL Layer objects
bulklayer/bl.o: bulklayer/bl.c
	$(CC) $(CFLAGS) $(INCLUDES) -c bulklayer/bl.c -o bulklayer/bl.o
bulklayer/blbulk.o: bulklayer/blbulk.c
	$(CC) $(CFLAGS) $(INCLUDES) -c bulklayer/blbulk.c -o bulklayer/blbulk.o
bulklayer/blsearch.o: bulklayer/blsearch.c
	$(CC) $(CFLAGS) $(INCLUDES) -c bulklayer/blsearch.c -o bulklayer/blsearch.o
bulklayer/blscan.o: bulklayer/blscan.c
	$(CC) $(CFLAGS) $(INCLUDES) -c bulklayer/blscan.c -o bulklayer/blscan.o
bulklayer/blprint.o: bulklayer/blprint.c
	$(CC) $(CFLAGS) $(INCLUDES) -c bulklayer/blprint.c -o bulklayer/blprint.o

# Clean up all generated object files and executables
clean:
	rm -f test_indexing $(OBJS)

.PHONY: all clean