# Compiler to use
CXX = g++

# Compiler flags
# -std=c++17: Use the C++17 standard
# -Wall: Turn on all common warnings (good practice)
# -g: Include debugging information
CXXFLAGS = -std=c++17 -Wall -g

# The final executable name
TARGET = matching_engine

# All .cpp source files
SRCS = main.cpp orderbook.cpp MatchingEngine.cpp Persistence.cpp Logger.cpp

# All .o (object) files, created from the .cpp files
OBJS = $(SRCS:.cpp=.o)

# The default rule (what happens when you just type "make")
# Build the target executable
all: $(TARGET)

# Rule to link the final executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Generic rule to compile a .cpp file into a .o file
# -c: Compile only, don't link
# $<: The source file (e.g., main.cpp)
# $@: The target file (e.g., main.o)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Tells make that "all" and "clean" are not actual files
.PHONY: all clean

