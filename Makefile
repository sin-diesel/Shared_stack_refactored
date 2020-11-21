CC = gcc
IDIR = ./include
CCFLAGS = -g -Wall -std=c11 -I $(IDIR)
SRC_DIR = source
SOURCES = stack.c program.c
HEADERS = stack.h
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLES = $(SOURCES:.c=)

.PHONY: all
all: $(OBJECTS)
	$(CC) $(CCFLAGS) $(OBJECTS) -o program

stack.o: $(SRC_DIR)/stack.c
	$(CC) $(CCFLAGS) $(SRC_DIR)/stack.c -c -o stack.o

program.o: $(SRC_DIR)/program.c
	$(CC) $(CCFLAGS) $(SRC_DIR)/program.c -c -o program.o	

.PHONY: clean
clean:
	rm *.o 