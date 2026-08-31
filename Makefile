# 1. Define your compiler and required build flags
CXX = g++
CXXFLAGS = -Wall -std=c++17 -Iinclude
LDFLAGS = -Llib -lraylib -lopengl32 -lgdi32 -lwinmm -Wl,--defsym,stat64i32=_stat64

# 2. Pattern Rule: Tells Make how to build ANY execution file from a matching .cpp file
# '%' acts as a wildcard. If you type 'make radio', % becomes 'radio'
%: %.cpp
	$(CXX) $< -o $@.exe $(CXXFLAGS) $(LDFLAGS)

# 3. Clean target to wipe out all generated .exe files
clean:
	del *.exe
