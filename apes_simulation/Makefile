# ============================================================
# Makefile for Apes Collecting Bananas Simulation
# ============================================================
# Usage:
#   make          - Build the simulation and OpenGL viewer
#   make clean    - Remove build artifacts
#   make run      - Build and run the simulation
#   make viewer   - Build only the OpenGL viewer
#   make debug    - Build with debug symbols for gdb
# ============================================================

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I./include

# Detect OS for platform-specific flags
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    # macOS - no librt needed, use OpenGL framework
    LDFLAGS = -pthread
    GL_FLAGS = -framework OpenGL -framework GLUT
else
    # Linux - use librt and standard OpenGL libraries
    LDFLAGS = -pthread -lrt
    GL_FLAGS = -lGL -lGLU -lglut -lm
endif

# Debug flags (use 'make debug' to enable)
DEBUG_FLAGS = -g -DDEBUG

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

# Source files for main simulation
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/config.c \
       $(SRC_DIR)/utils.c \
       $(SRC_DIR)/maze.c \
       $(SRC_DIR)/family.c \
       $(SRC_DIR)/sem_wrapper.c

# Object files
OBJS = $(OBJ_DIR)/main.o \
       $(OBJ_DIR)/config.o \
       $(OBJ_DIR)/utils.o \
       $(OBJ_DIR)/maze.o \
       $(OBJ_DIR)/family.o \
       $(OBJ_DIR)/sem_wrapper.o

# Target executables
TARGET = apes_simulation
VIEWER = apes_viewer

# Default config file
CONFIG = simulation.conf

# ============================================================
# Targets
# ============================================================

# Default target: build both simulation and viewer
all: $(OBJ_DIR) $(TARGET) $(VIEWER)

# Create object directory
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Link object files to create main executable
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Build OpenGL viewer
$(VIEWER): $(SRC_DIR)/viewer.c $(INC_DIR)/shared_data.h
	@echo "Compiling OpenGL viewer..."
	$(CC) $(CFLAGS) $(SRC_DIR)/viewer.c -o $(VIEWER) $(GL_FLAGS)
	@echo "Build complete: $(VIEWER)"

# Build only viewer
viewer: $(OBJ_DIR) $(VIEWER)
	@echo "Viewer build complete!"

# Common header dependency - all source files include this
COMMON_H = $(INC_DIR)/local.h

# Compile source files to object files
$(OBJ_DIR)/main.o: $(SRC_DIR)/main.c $(COMMON_H)
	@echo "Compiling main.c..."
	$(CC) $(CFLAGS) -c $(SRC_DIR)/main.c -o $(OBJ_DIR)/main.o

$(OBJ_DIR)/config.o: $(SRC_DIR)/config.c $(COMMON_H)
	@echo "Compiling config.c..."
	$(CC) $(CFLAGS) -c $(SRC_DIR)/config.c -o $(OBJ_DIR)/config.o

$(OBJ_DIR)/utils.o: $(SRC_DIR)/utils.c $(COMMON_H)
	@echo "Compiling utils.c..."
	$(CC) $(CFLAGS) -c $(SRC_DIR)/utils.c -o $(OBJ_DIR)/utils.o

$(OBJ_DIR)/maze.o: $(SRC_DIR)/maze.c $(COMMON_H)
	@echo "Compiling maze.c..."
	$(CC) $(CFLAGS) -c $(SRC_DIR)/maze.c -o $(OBJ_DIR)/maze.o

$(OBJ_DIR)/family.o: $(SRC_DIR)/family.c $(COMMON_H)
	@echo "Compiling family.c..."
	$(CC) $(CFLAGS) -c $(SRC_DIR)/family.c -o $(OBJ_DIR)/family.o

$(OBJ_DIR)/sem_wrapper.o: $(SRC_DIR)/sem_wrapper.c $(COMMON_H)
	@echo "Compiling sem_wrapper.c..."
	$(CC) $(CFLAGS) -c $(SRC_DIR)/sem_wrapper.c -o $(OBJ_DIR)/sem_wrapper.o

# Build with debug symbols
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean all
	@echo "Debug build complete. Use 'gdb ./$(TARGET)' to debug."

# Run simulation with OpenGL viewer (default)
run: all
	@echo "Starting OpenGL viewer in background..."
	./$(VIEWER) &
	@sleep 1
	@echo "Starting simulation..."
	./$(TARGET) $(CONFIG)

# Run the simulation (terminal only, no GUI)
run-terminal: $(TARGET)
	@echo "Running simulation (terminal only)..."
	./$(TARGET) $(CONFIG)

# Run with custom config file
# Usage: make run-config CONFIG=myconfig.conf
run-config: all
	@echo "Running simulation with config: $(CONFIG)"
	./$(TARGET) $(CONFIG)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET) $(VIEWER)
	@echo "Clean complete."

# Clean shared memory (in case of crash)
clean-shm:
	@echo "Cleaning shared memory..."
	ipcrm -M 0x1234 2>/dev/null || true
	@echo "Shared memory cleaned."

# Full clean (including shared memory)
distclean: clean clean-shm

# Show help
help:
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║        Apes Simulation - Build System                          ║"
	@echo "╚════════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build simulation + OpenGL viewer"
	@echo "  make viewer   - Build only the OpenGL viewer"
	@echo "  make debug    - Build with debug symbols for gdb"
	@echo "  make run      - Run with OpenGL visualization (default)"
	@echo "  make run-terminal - Run simulation (terminal only, no GUI)"
	@echo "  make run-config CONFIG=file.conf - Run with custom config"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make clean-shm - Clean shared memory segments"
	@echo "  make distclean - Full clean (build + shared memory)"
	@echo ""
	@echo "To use OpenGL viewer separately:"
	@echo "  Terminal 1: ./apes_simulation simulation.conf"
	@echo "  Terminal 2: ./apes_viewer"

# Phony targets
.PHONY: all clean debug run run-terminal run-config clean-shm distclean help viewer

