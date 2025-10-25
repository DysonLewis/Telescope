# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -Wall -O2 -march=native -fno-fast-math
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system

# Target executable
TARGET = optic_raytracer

# Source files
SOURCES = optic_raytracer.cpp

# Header files (for dependency tracking)
HEADERS = Ray.h Mirror.h Optimizer.h

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default target: build
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

# Compile source files to object files (depends on headers)
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Rebuild from scratch
rebuild: clean all

# Install headers (optional, for future library use)
install-headers:
	mkdir -p include
	cp $(HEADERS) include/

.PHONY: all clean rebuild install-headers