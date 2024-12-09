# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++17 -I$(INCDIR) -O3
LDFLAGS = -lboost_system -lboost_date_time 

# Directories
INCDIR = include
SRCDIR = src
OBJDIR = obj

# Executable names
TARGETS = InstancesServerApp APIServerApp TunnelServerApp 

# Source files in src/ and object files in obj/
SOURCES = $(foreach dir, $(shell find $(SRCDIR) -type d), $(wildcard $(dir)/*.cpp))
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Main source files for instances, API, and tunnel servers
INSTANCES_SERVER_APP = InstancesServerApp.cpp
API_SERVER_APP = APIServerApp.cpp
TUNNEL_SERVER_APP = TunnelServerApp.cpp

# Object files for instances, API, and tunnel servers
INSTANCES_SERVER_OBJECT = $(INSTANCES_SERVER_APP:.cpp=.o)
API_SERVER_OBJECT = $(API_SERVER_APP:.cpp=.o)
TUNNEL_SERVER_OBJECT = $(TUNNEL_SERVER_APP:.cpp=.o)

# Default rule to build all executables
all: $(TARGETS)

# Rule to build the instances server executable
InstancesServerApp: $(OBJECTS) $(INSTANCES_SERVER_OBJECT)
	$(CXX) $(OBJECTS) $(INSTANCES_SERVER_OBJECT) -o $@ $(LDFLAGS)

# Rule to build the API server executable
APIServerApp: $(OBJECTS) $(API_SERVER_OBJECT)
	$(CXX) $(OBJECTS) $(API_SERVER_OBJECT) -o $@ $(LDFLAGS)

# Rule to build the tunnel server executable
TunnelServerApp: $(OBJECTS) $(TUNNEL_SERVER_OBJECT)
	$(CXX) $(OBJECTS) $(TUNNEL_SERVER_OBJECT) -o $@ $(LDFLAGS)

# Aliases for convenience
instances: InstancesServerApp
api: APIServerApp
tunnel: TunnelServerApp

# Rule to compile source files in src/ into obj/ (create directory structure if needed)
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)  # Ensure the parent directory exists
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to compile main source files directly into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule to remove object files and executables
clean:
	rm -rf $(OBJDIR) $(TARGETS) $(INSTANCES_SERVER_OBJECT) $(API_SERVER_OBJECT) $(TUNNEL_SERVER_OBJECT)

.PHONY: all clean instances api tunnel
