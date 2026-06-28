CXX     := clang++
TARGET  := plotter
SRCS    := plotter.cpp

CFLAGS  := -std=c++23 -Wall -Wextra -I/opt/homebrew/include
LDFLAGS := -L/opt/homebrew/lib -lraylib \
		 -framework Cocoa -framework IOKit -framework CoreVideo -framework OpenGL

$(TARGET): $(SRCS)
	$(CXX) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

parser: parser.cpp
	$(CXX) $(CFLAGS) parser.cpp -o parser

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f parser plotter

.PHONY: run clean
