#
# Makefile for Database Assignment - Objective 3 Evaluation
#
# This Makefile builds the 'obj3_eval' executable, which links
# the PF, AM, and SP layers with the new code for Objective 3.
#

# Compiler and C Flags
# Use C89 standard as specified in the provided Makefiles.
# Add include paths for all necessary layers.
CC = gcc -std=c89 -Wno-implicit-function-declaration
CFLAGS = -g -Wall -I. -I./amlayer -I./pflayer -I./splayer

# --- Source Directories ---
PF_DIR = ./pflayer
AM_DIR = ./amlayer
SP_DIR = ./splayer

# --- PF Layer Objects ---
PF_OBJS = \
	$(PF_DIR)/pf.o \
	$(PF_DIR)/buf.o \
	$(PF_DIR)/hash.o

# --- AM Layer Objects ---
AM_OBJS = \
	$(AM_DIR)/am.o \
	$(AM_DIR)/amfns.o \
	$(AM_DIR)/amsearch.o \
	$(AM_DIR)/aminsert.o \
	$(AM_DIR)/amstack.o \
	$(AM_DIR)/amglobals.o \
	$(AM_DIR)/amscan.o \
	$(AM_DIR)/amprint.o

# --- SP Layer Objects ---
SP_OBJS = \
	$(SP_DIR)/sp.o

# --- Objective 3 Objects ---
# These are the new files you are adding
OBJ3_OBJS = \
	obj3_eval.o \
	am_bulkload.o

# --- All Objects ---
ALL_OBJS = $(OBJ3_OBJS) $(SP_OBJS) $(AM_OBJS) $(PF_OBJS)

# --- Target Executable ---
TARGET = obj3_eval

# --- Build Rules ---

# Default target
all: $(TARGET)

# Link the final executable
$(TARGET): $(ALL_OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(ALL_OBJS) -lm

# --- Compilation Rules for Objective 3 ---
obj3_eval.o: obj3_eval.c student_data.h am_bulkload.h amlayer/am.h splayer/sp.h pflayer/pf.h
	$(CC) $(CFLAGS) -c obj3_eval.c -o obj3_eval.o

am_bulkload.o: am_bulkload.c am_bulkload.h student_data.h amlayer/am.h pflayer/pf.h
	$(CC) $(CFLAGS) -c am_bulkload.c -o am_bulkload.o

# --- Compilation Rules for Existing Layers ---
# These rules compile the .c files from their respective subdirectories

$(PF_DIR)/%.o: $(PF_DIR)/%.c $(PF_DIR)/pf.h $(PF_DIR)/pftypes.h
	$(CC) $(CFLAGS) -c $< -o $@

$(AM_DIR)/%.o: $(AM_DIR)/%.c $(AM_DIR)/am.h $(AM_DIR)/pf.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SP_DIR)/%.o: $(SP_DIR)/%.c $(SP_DIR)/sp.h $(SP_DIR)/sptype.h $(PF_DIR)/pf.h
	$(CC) $(CFLAGS) -c $< -o $@

# --- Clean Rule ---
clean:
	rm -f $(TARGET) *.o $(PF_DIR)/*.o $(AM_DIR)/*.o $(SP_DIR)/*.o *~
	rm -f student.db student.db.0

.PHONY: all clean