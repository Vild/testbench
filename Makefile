CXX := g++
CXXFLAGS := -Wall -Iinclude -DDEBUG -std=c++1z -ggdb $(shell pkg-config --cflags gl glew sdl2 vulkan)
LFLAGS := $(shell pkg-config --libs gl glew sdl2 vulkan)

SRC := gl_testbench/
OBJ := obj/

TARGET := testbench

SOURCES := $(shell find $(SRC) -iname "*.cpp")
HEADERS := $(shell find $(SRC) -iname "*.h")
OBJECTS := $(patsubst $(SRC)%.cpp, $(OBJ)%.o, $(SOURCES))

.PHONY: new all clean

all: new

new:
	$(MAKE) clean
	$(MAKE) $(TARGET)

run: all
	@./$(TARGET)

$(OBJ)%.o: $(SRC)%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $< -o $@ $(CXXFLAGS)

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LFLAGS)

clean:
	$(RM) -rf $(TARGET) $(OBJ)
