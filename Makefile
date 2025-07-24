# Easy-to-understand Makefile for VCS
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Wno-format-truncation
TARGET = vcs

# List all source files
SOURCES = main.c repo.c fileops.c version.c metadata.c utils.c

# Convert .c files to .o files
OBJECTS = main.o repo.o fileops.o version.o metadata.o utils.o

# Build the main program
all: $(TARGET)

# Link all object files to create the final executable
$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) -lssl -lcrypto

# Compile each source file to object file
# Each .o file depends on its .c file AND the header file
main.o: main.c vcs.h
	$(CC) $(CFLAGS) -c main.c

repo.o: repo.c vcs.h
	$(CC) $(CFLAGS) -c repo.c

fileops.o: fileops.c vcs.h
	$(CC) $(CFLAGS) -c fileops.c

version.o: version.c vcs.h
	$(CC) $(CFLAGS) -c version.c

metadata.o: metadata.c vcs.h
	$(CC) $(CFLAGS) -c metadata.c

utils.o: utils.c vcs.h
	$(CC) $(CFLAGS) -c utils.c

# Remove all compiled files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Declare 'all' and 'clean' as phony targets
# This tells 'make' that these are not actual files, but just labels for commands
.PHONY: all clean
