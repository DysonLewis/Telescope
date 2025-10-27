# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -Wall -O2 -march=native -fno-fast-math
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system

# Target executables
TARGET = optic_raytracer
BATCH_TARGET = batch_optimize

# Header files (for dependency tracking)
HEADERS = Ray.h Mirror.h Camera.h Optimizer.h BatchOptimizer.h ConfigBuilder.h

# Default target: build both programs
all: $(TARGET) $(BATCH_TARGET)

# Build the GUI ray tracer (now includes BatchOptimizer.o)
$(TARGET): optic_raytracer.o Ray.o Mirror.o Camera.o Optimizer.o BatchOptimizer.o
	$(CXX) $^ $(LDFLAGS) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

# Build the batch optimizer (also needs SFML for sf::Vector2f, sf::Color, etc.)
$(BATCH_TARGET): batch_optimize_main.o Ray.o Mirror.o Camera.o Optimizer.o BatchOptimizer.o
	$(CXX) $^ $(LDFLAGS) -o $(BATCH_TARGET)
	@echo "Build complete: $(BATCH_TARGET)"

# Compile source files to object files (depends on headers)
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f *.o $(TARGET) $(BATCH_TARGET)

# Rebuild from scratch
rebuild: clean all

# Install headers (optional, for future library use)
install-headers:
	mkdir -p include
	cp $(HEADERS) include/

.PHONY: all clean rebuild install-headers