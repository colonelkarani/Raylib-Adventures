# =====================================================================================
#  Makefile -- builds "or_defense" against a locally-built raylib.
#  Usage:
#    make RAYLIB_DIR=/path/to/raylib
#  Defaults assume raylib was cloned as a sibling of this project's parent directory
#  (see the accompanying BUILD.md for the exact steps used during development).
# =====================================================================================
RAYLIB_DIR ?= ../raylib
CXX ?= g++
CXXFLAGS := -std=c++17 -O2 -Wall -Iinclude -I$(RAYLIB_DIR)/src
LDFLAGS  := -L$(RAYLIB_DIR)/src -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:src/%.cpp=build/%.o)
BIN := or_defense

all: $(BIN)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

$(BIN): $(OBJ)
	$(CXX) $(OBJ) -o $(BIN) $(LDFLAGS)

run: $(BIN)
	./$(BIN)

clean:
	rm -rf build $(BIN)

.PHONY: all run clean
