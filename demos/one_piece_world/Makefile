# One Piece World 3D Visualizer
CXX      = g++
CXXFLAGS = -std=c++11 -O2 -Wall
LDFLAGS  = -lfreeglut -lopengl32 -lglu32

SRCS = main.cpp data.cpp stars.cpp globe.cpp journey.cpp hud.cpp
HDRS = globals.h noise.h data.h stars.h globe.h journey.h hud.h
TARGET = one_piece_world.exe

all: $(TARGET)
	@echo "Build complete: $(TARGET)"

$(TARGET): $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
