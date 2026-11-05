CXX := g++

# Optimised build by default; `make DEBUG=1` for an unoptimised build with debug symbols.
# (Switch between them with `make clean` first, objects are not tracked per configuration.)
ifeq ($(DEBUG),1)
    OPT_FLAGS := -O0 -g
else
    OPT_FLAGS := -O2
endif

CXX_FLAGS := -std=c++20 $(OPT_FLAGS) -Wall -Wpedantic -Wextra -pedantic -MMD -MP $(shell pkg-config --cflags sdl3)
# OpenGL entry points are resolved at runtime through SDL_GL_GetProcAddress (see engine/gl_loader),
# so there is no link-time dependency on libGL.
LINKER_FLAGS := $(shell pkg-config --libs sdl3) -lm

SRC_FILES := $(wildcard src/*.cpp src/random/*.cpp src/engine/*.cpp)
OBJ_FILES := $(SRC_FILES:.cpp=.o)
DEP_FILES := $(OBJ_FILES:.o=.d)

TARGET := kwirbia

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) $(LINKER_FLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXX_FLAGS) -c $< -o $@

-include $(DEP_FILES)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJ_FILES) $(DEP_FILES)
