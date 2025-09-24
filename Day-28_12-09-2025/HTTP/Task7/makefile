# ========================================================
# Makefile for simple HTTP server (Task 1)
# ========================================================
# This Makefile automates the build, run, and clean process 
# for a simple C-based HTTP server.
#
# Targets:
#   make        → Compiles the server
#   make run    → Runs the compiled server
#   make clean  → Removes the compiled binary
# ========================================================

# Compiler to use
CC = gcc

# Compiler flags:
# -Wall   → Enable all common warnings
# -Wextra → Enable extra (more strict) warnings
# -g      → Include debugging information (useful with gdb/valgrind)
CFLAGS = -Wall -Wextra -g -O2 -pthread

# Name of the final output binary
TARGET = server

# ------------------------
# Default target (make all)
# This will compile the code into the TARGET executable.
# ------------------------
all: $(TARGET)

# Rule to build the target (server) from server.c
$(TARGET): server.c
	$(CC) $(CFLAGS) -o $(TARGET) server.c

# ------------------------
# Run target (make run)
# Runs the compiled server binary.
# ------------------------
run: $(TARGET)
	./$(TARGET)

# ------------------------
# Clean target (make clean)
# Deletes the compiled binary so you can rebuild fresh.
# ------------------------
clean:
	rm -f $(TARGET)
