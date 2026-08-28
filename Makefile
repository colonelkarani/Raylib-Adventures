# 1. Force make to completely disable all built-in implicit rules
.SUFFIXES:
% : %,v
% : RCS/%,v
% : RCS/%
% : s.%
% : SCCS/s.%

# Compiler and Linker settings
CC = g++
CFLAGS = -Wall -std=c++17 -Iinclude
LDFLAGS = -Llib -lraylib -lopengl32 -lgdi32 -lwinmm '-Wl,--defsym,stat64i32=_stat64'

TARGET = main.exe
SRC = main.cpp

# Make 'all' the default when you just type 'make'
all: $(TARGET)

# The actual manual build instructions
$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LDFLAGS)

# Force 'make main' to just run our build instead of its built-in rule
main: $(TARGET)

# Clean rule
clean:
	del $(TARGET)

