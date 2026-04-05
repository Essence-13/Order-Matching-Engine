# Makefile for the Order Matching Engine

# Compiler
CXX = g++

# Compiler flags
# -std=c++17: Use the C++17 standard
# -Wall: Turn on all warnings
# -O3: Aggressive optimization (for release/benchmark build)
# -g: Add debug symbols (for debug build)
CXXFLAGS = -std=c++17 -Wall -O3

# Use this line for debugging instead:
# CXXFLAGS = -std=c++17 -Wall -g

# Target executable name
TARGET = matching_engine

# Source files
# We list all .cpp files that need to be compiled
SRCS = main.cpp \
       Logger.cpp \
       Persistence.cpp \
       MatchingEngine.cpp \
       OrderBook.cpp

# Object files
# This rule automatically converts the .cpp list to a .o list
# e.g., main.cpp -> main.o
OBJS = $(SRCS:.cpp=.o)

# The default rule to build the target
# This is what runs when you just type 'make'
# It depends on all the object files
all: $(TARGET)

# Rule to link the target executable
# It depends on all the object files
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

benchmark:
	g++ -std=c++17 -Wall -O3 -DBENCHMARK_MODE -o matching_engine_bench *.cpp

# Generic rule to compile a .cpp file into a .o file
# This says "to make a .o file, you need the .cpp file with the same name"
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# The 'clean' rule to remove compiled files
# This is for Windows (PowerShell/CMD). The '@' suppresses errors if files don't exist.
# clean:
# 	-@del $(OBJS) $(TARGET) 2>nul

# This is the Linux/macOS version. Use this if you are in Git Bash.
clean:
	rm -f $(OBJS) $(TARGET)

