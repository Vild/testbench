CC := gcc
CXX := g++
CFLAGS := -Iinclude -std=gnu11 -ggdb $(shell pkg-config --cflags gl glew sdl2 vulkan)
CXXFLAGS := -Iinclude -std=c++1z -ggdb $(shell pkg-config --cflags gl glew sdl2 vulkan)
LFLAGS := $(shell pkg-config --libs gl glew sdl2 vulkan)

SRC := gl_testbench
OBJ := obj

TARGET := testbench

SOURCES := $(shell find $(SRC) -iname "*.cpp")
HEADERS := $(shell find $(SRC) -iname "*.h")
OBJECTS := $(patsubst $(SRC)/%.cpp, $(OBJ)/%.o, $(SOURCES))

DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
$(shell find gl_testbench/ -type d ! -iname "Debug" ! -iname "Release" | sed "s/gl_testbench/$(DEPDIR)/g" | xargs mkdir -p >/dev/null)
$(shell find gl_testbench/ -type d ! -iname "Debug" ! -iname "Release" | sed "s/gl_testbench/$(OBJ)/g" | xargs mkdir -p >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Tdep

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = @mv -f $(DEPDIR)/$*.Tdep $(DEPDIR)/$*.dep && touch $@

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LFLAGS)

$(OBJ)/%.o: $(SRC)/%.c
$(OBJ)/%.o: $(SRC)/%.c $(DEPDIR)/%.dep
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(OBJ)/%.o: $(SRC)/%.cpp
$(OBJ)/%.o: $(SRC)/%.cpp $(DEPDIR)/%.dep
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(DEPDIR)/%.dep: ;
.PRECIOUS: $(DEPDIR)/%.dep

clean:
	$(RM) -rf $(TARGET) $(OBJ) $(DEPDIR)

include $(wildcard $(patsubst %,$(DEPDIR)/%.dep,$(basename $(SRCS))))
