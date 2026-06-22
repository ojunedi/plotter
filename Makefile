CXX     := c++
TARGET  := plotter
SRCS    := plotter.cpp

CFLAGS  := -std=c++17 -Wall -Wextra -I/opt/homebrew/include

parser: parser.cpp
	$(CXX) $(CFLAGS) parser.cpp -o parser
LDFLAGS := -L/opt/homebrew/lib -lraylib \
		 -framework Cocoa -framework IOKit -framework CoreVideo -framework OpenGL

$(TARGET): $(SRCS)
	$(CXX) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: run clean
