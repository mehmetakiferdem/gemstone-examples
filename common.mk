# Default object files (can be extended in project Makefile)
C_OBJECTS ?= $(addprefix $(BUILDDIR)/,$(C_SOURCES:.c=.o))
CXX_OBJECTS ?= $(addprefix $(BUILDDIR)/,$(CXX_SOURCES:.cpp=.o))

CFLAGS += \
	--std=gnu11 \
	-pedantic \
	-Wall \
	-Werror \
	-Wextra \
	-Wfloat-equal \
	-Wno-error=unused-parameter \
	-Wpointer-arith \
	-Wshadow \
	-Wswitch-enum \
	-Wundef \
	-Wwrite-strings

CXXFLAGS += \
	--std=c++17 \
	-pedantic \
	-Wall \
	-Weffc++ \
	-Werror \
	-Wextra \
	-Wno-error=effc++ \
	-Wno-error=unused-parameter \

# Use CXX for linking when C++ objects are present, otherwise use CC
LINKER = $(if $(CXX_OBJECTS),$(CXX),$(CC))
LINK_FLAGS = $(if $(CXX_OBJECTS),$(CXXFLAGS),$(CFLAGS))

# Ensure build directory exists
$(shell mkdir -p $(BUILDDIR))

# Default build rule for target
$(BUILDDIR)/$(TARGET): $(C_OBJECTS) $(CXX_OBJECTS)
	$(LINKER) $(LINK_FLAGS) -o $@ $^ $(LDFLAGS)

# Pattern rule for compiling .c files
$(BUILDDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern rule for compiling .cpp files
$(BUILDDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(BUILDDIR)/$(TARGET) $(C_OBJECTS) $(CXX_OBJECTS)

.PHONY: clean
