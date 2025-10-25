# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -Wall -O2 -march=native -fno-fast-math
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system

# Target executable
TARGET = compiled/optic_raytracer

# Source files (now includes all .cpp files)
SOURCES = Ray.cpp Mirror.cpp Camera.cpp Optimizer.cpp optic_raytracer.cpp

# Header files (for dependency tracking)
HEADERS = Ray.h Mirror.h Camera.h Optimizer.h

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default target: build
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	@mkdir -p compiled
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

# Compile source files to object files with proper dependencies
Ray.o: Ray.cpp Ray.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

Mirror.o: Mirror.cpp Mirror.h Ray.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

Camera.o: Camera.cpp Camera.h Mirror.h Ray.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

Optimizer.o: Optimizer.cpp Optimizer.h Mirror.h Camera.h Ray.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

optic_raytracer.o: optic_raytracer.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete"

# Rebuild from scratch
rebuild: clean all

# Install headers (optional, for future library use)
install-headers:
	mkdir -p include
	cp $(HEADERS) include/
	@echo "Headers installed to include/"

# Run the program
run: $(TARGET)
	./$(TARGET)

# Debug build (with debug symbols, no optimization)
debug: CXXFLAGS = -std=c++20 -Wall -g -O0
debug: clean all
	@echo "Debug build complete"

# Show build information
info:
	@echo "Compiler: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "Sources: $(SOURCES)"
	@echo "Objects: $(OBJECTS)"
	@echo "Target: $(TARGET)"

# Check for SFML installation
check-sfml:
	@echo "Checking SFML installation..."
	@pkg-config --exists sfml-all && echo "SFML found!" || echo "SFML not found. Please install SFML."
	@pkg-config --modversion sfml-all 2>/dev/null || echo "Version info unavailable"

.PHONY: all clean rebuild install-headers run debug info check-sfml