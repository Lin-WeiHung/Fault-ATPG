# Compiler
CC = g++

# Compiler flags
CFLAGS = -Wall -g

# Source files
PARSER = test_parser.cpp fault_parser.cpp

# Object files
OBJS = test_parser.o fault_parser.o

# Executable name
EXEC = program

# Default target
all: $(EXEC)

# Link object files to create executable
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files to object files
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(EXEC)

.PHONY: all clean