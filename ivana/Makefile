# =====================================================================================
#  Makefile -- builds "or_defense" against a locally-built raylib.
#  Usage:
#    make RAYLIB_DIR=/path/to/raylib
#  Defaults assume raylib was cloned as a sibling of this project's parent directory.
#  Works on Linux, macOS, and Windows (MSYS2/MinGW) -- the link flags are chosen
#  automatically based on the detected OS below.
# =====================================================================================
RAYLIB_DIR ?= ../raylib
CXX ?= g++
CXXFLAGS := -std=c++17 -O2 -Wall -Iinclude -I$(RAYLIB_DIR)/src

# ---------------------------------------------------------------------------- Platform
# raylib itself needs different system libraries linked in on each OS. Detect via
# uname where available (Linux/macOS/MSYS2 all provide it); fall back to Windows
# defaults if uname is missing entirely (plain "cmd.exe" with no MSYS2 tools on PATH).
UNAME_S := $(shell uname -s 2>/dev/null)

ifeq ($(OS),Windows_NT)
    ifneq (,$(findstring MINGW,$(UNAME_S))$(findstring MSYS,$(UNAME_S)))
        # MSYS2/MinGW shell: use its GL/Win32 system libs.
        LDFLAGS := -L$(RAYLIB_DIR)/src -lraylib -lopengl32 -lgdi32 -lwinmm
        BIN := or_defense.exe
    else
        # Windows without a recognized POSIX shell layer.
        LDFLAGS := -L$(RAYLIB_DIR)/src -lraylib -lopengl32 -lgdi32 -lwinmm
        BIN := or_defense.exe
    endif
else ifeq ($(UNAME_S),Darwin)
    LDFLAGS := -L$(RAYLIB_DIR)/src -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
    BIN := or_defense
else
    # Linux and other POSIX systems.
    LDFLAGS := -L$(RAYLIB_DIR)/src -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    BIN := or_defense
endif

SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:src/%.cpp=build/%.o)

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
	rm -rf build $(BIN) or_defense or_defense.exe

.PHONY: all run clean
