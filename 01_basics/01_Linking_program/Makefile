# Variables
CC = gcc
CFLAGS = -Wall -Wextra -I.
OBJ = file01.o file02.o
TARGET = mycode

# Default target
all: $(TARGET)

# Linking the object files to create the executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Compiling file01.c to object file
file01.o: file01.c file02.h
	$(CC) $(CFLAGS) -c file01.c -o file01.o

# Compiling file02.c to object file
file02.o: file02.c file02.h
	$(CC) $(CFLAGS) -c file02.c -o file02.o

# Clean up the build files
clean:
	rm -f $(OBJ) $(TARGET)

# Phony targets
.PHONY: all clean
