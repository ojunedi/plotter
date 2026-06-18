CC      := cc
TARGET  := main
SRCS    := $(wildcard *.c)

CFLAGS  := -std=c17 -Wall -Wextra -I/opt/homebrew/include
LDFLAGS := -L/opt/homebrew/lib -lraylib \
		 -framework Cocoa -framework IOKit -framework CoreVideo -framework OpenGL

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: run clean
