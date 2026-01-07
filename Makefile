# Makefile for Ape Banana Simulation Project
# ENCS4330 - Real-Time Applications & Embedded Systems
# Birzeit University - 2025/2026

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
DEBUG_FLAGS = -g -DDEBUG -O0 -fsanitize=thread
LIBS = -lpthread -lglut -lGLU -lGL -lm

TARGET = ape_simulation
SOURCES = ape_simulation.c
OBJECTS = $(SOURCES:.c=.o)
CONFIG = config.txt

.PHONY: all run clean debug check help test valgrind

all: $(TARGET)
	@echo "========================================="
	@echo "✓ Build completed successfully!"
	@echo "Run with: ./$(TARGET) $(CONFIG)"
	@echo "========================================="

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)
	@echo "✓ Executable created"

%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	@echo "Starting simulation..."
	@echo "========================================="
	./$(TARGET) $(CONFIG)

clean:
	@echo "Cleaning build artifacts..."
	@rm -f $(TARGET) $(OBJECTS) *.o core vgcore.*
	@echo "✓ Clean complete"

debug: CFLAGS = $(DEBUG_FLAGS)
debug: clean $(TARGET)
	@echo "✓ Debug build with ThreadSanitizer enabled"
	@echo "Run with: ./$(TARGET) $(CONFIG)"

check:
	@echo "Checking required libraries..."
	@command -v pkg-config >/dev/null 2>&1 || { echo "pkg-config not found"; exit 1; }
	@pkg-config --exists glut && echo "  ✓ GLUT found" || echo "  ✗ GLUT missing: sudo apt-get install freeglut3-dev"
	@pkg-config --exists gl && echo "  ✓ OpenGL found" || echo "  ✗ OpenGL missing: sudo apt-get install libgl1-mesa-dev"
	@pkg-config --exists glu && echo "  ✓ GLU found" || echo "  ✗ GLU missing: sudo apt-get install libglu1-mesa-dev"
	@echo "✓ Check complete"

test: $(TARGET)
	@echo "Running quick test simulation (20 seconds)..."
	@echo "MAX_SIMULATION_TIME=20" > test_config.txt
	@grep -v "MAX_SIMULATION_TIME" $(CONFIG) >> test_config.txt
	@./$(TARGET) test_config.txt
	@rm -f test_config.txt

valgrind: $(TARGET)
	@echo "Running with Valgrind memory checker..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET) $(CONFIG)

help:
	@echo "========================================="
	@echo "Ape Banana Simulation - Makefile Help"
	@echo "========================================="
	@echo "Available targets:"
	@echo "  make           - Build the project (optimized)"
	@echo "  make run       - Build and run simulation"
	@echo "  make clean     - Remove build artifacts"
	@echo "  make debug     - Build with debug symbols & ThreadSanitizer"
	@echo "  make check     - Verify required libraries"
	@echo "  make test      - Quick 20s test run"
	@echo "  make valgrind  - Run with memory leak detection"
	@echo "  make help      - Show this help"
	@echo "========================================="
	@echo "Configuration:"
	@echo "  Edit $(CONFIG) to adjust simulation parameters"
	@echo "========================================="
