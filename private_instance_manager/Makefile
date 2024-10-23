# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++17 -I$(INCDIR) -O3
LDFLAGS = -lboost_system -lboost_date_time 

# Directories
INCDIR = include
SRCDIR = src
OBJDIR = obj

# Executable names
TARGETS = dispatcher tunnel

# Source files in src/ and object files in obj/
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Main source files for dispatcher and tunnel
DISPATCHER_MAIN = dispatcher.cpp
TUNNEL_MAIN = tunnel.cpp

# Object files for dispatcher and tunnel
DISPATCHER_OBJECT = $(DISPATCHER_MAIN:.cpp=.o)
TUNNEL_OBJECT = $(TUNNEL_MAIN:.cpp=.o)

# Default rule to build both executables
all: $(TARGETS)

# Rule to build the dispatcher executable
dispatcher: $(OBJECTS) $(DISPATCHER_OBJECT)
	$(CXX) $(OBJECTS) $(DISPATCHER_OBJECT) -o dispatcher $(LDFLAGS)

# Rule to build the tunnel executable
tunnel: $(OBJECTS) $(TUNNEL_OBJECT)
	$(CXX) $(OBJECTS) $(TUNNEL_OBJECT) -o tunnel $(LDFLAGS)

# Rule to compile source files in src/ into obj/
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to compile main source files directly into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create the obj directory if it doesn't exist
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Clean rule to remove object files and executables
clean:
	rm -rf $(OBJDIR) dispatcher tunnel $(DISPATCHER_OBJECT) $(TUNNEL_OBJECT)

.PHONY: all clean
