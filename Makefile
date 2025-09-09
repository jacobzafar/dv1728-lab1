# Makefile for UDP/TCP Client with Binary/Text Protocol Support

# Compiler and flags
CXX = g++
CC = gcc
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic
CFLAGS = -std=c99 -Wall -Wextra -pedantic

# Debug flags (uncomment for debug build)
# CXXFLAGS += -DDEBUG -g
# CFLAGS += -DDEBUG -g

# Platform-specific linking
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)
ifeq ($(UNAME_S),Windows_NT)
    LDFLAGS += -lws2_32
else ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
    LDFLAGS += -lws2_32
else ifeq ($(findstring CYGWIN,$(UNAME_S)),CYGWIN)
    LDFLAGS += -lws2_32
endif

# Target executable
TARGET = client

# Source files
SOURCES_CPP = clientmain.cpp
SOURCES_C = calcLib.c

# Object files
OBJECTS = $(SOURCES_CPP:.cpp=.o) $(SOURCES_C:.c=.o)

# Headers
HEADERS = protocol.h calcLib.h

# Default target
all: $(TARGET)

# Build the client
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile C++ source files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile C source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build
debug: CXXFLAGS += -DDEBUG -g
debug: CFLAGS += -DDEBUG -g
debug: $(TARGET)

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET) $(TARGET).exe

# Test target (optional)
test: $(TARGET)
	@echo "Testing TCP TEXT protocol..."
	# Add test commands here when ready

# Install dependencies (for reference)
install:
	@echo "No dependencies to install for basic build"
	@echo "Requires: g++, gcc, standard C++ and C libraries"

# Help target
help:
	@echo "Available targets:"
	@echo "  all (default) - Build the client"
	@echo "  debug         - Build with debug flags"
	@echo "  clean         - Remove build artifacts"
	@echo "  test          - Run basic tests (when implemented)"
	@echo "  help          - Show this help message"

.PHONY: all debug clean test install help
