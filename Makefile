CXX := clang++
CXXFLAGS := -fsanitize=address -Wall -Iinclude -DDEBUG -O0 -std=c++1z -ggdb $(shell pkg-config --cflags gl glew sdl2 vulkan)
LFLAGS := -fsanitize=address $(shell pkg-config --libs gl glew sdl2 vulkan) -lstdc++fs

SRC := gl_testbench/
OBJ := obj/

TARGET := testbench

SOURCES := $(shell find $(SRC) -iname "*.cpp")
HEADERS := $(shell find $(SRC) -iname "*.h")
OBJECTS := $(patsubst $(SRC)%.cpp, $(OBJ)%.o, $(SOURCES))

.PHONY: new all clean

all: new

new:
	@$(MAKE) clean
	@bear $(MAKE) $(TARGET)

run: all
	@./$(TARGET)

$(OBJ)%.o: $(SRC)%.cpp
	@mkdir -p $(dir $@)
	+$(CXX) -c $< -o $@ $(CXXFLAGS)

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	+$(CXX) $(OBJECTS) -o $(TARGET) $(LFLAGS)

clean:
	@$(RM) -rf $(TARGET) $(OBJ)
