# ═══════════════════════════════════════════
#  DUNGEON ROGUELITE — Makefile
# ═══════════════════════════════════════════

CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Ilib -Igame
LDFLAGS  = -lfreeglut -lopengl32 -lglu32

# Source files
SRCS = game/main.cpp game/world.cpp game/renderer.cpp
TARGET = dungeon.exe

# Default target
all: $(TARGET)
	@echo "Build complete: $(TARGET)"

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Clean build artifacts
clean:
	rm -f $(TARGET)

# Run the game
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
