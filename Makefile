CC = gcc
CFLAGS = -Wall -g `pkg-config --cflags gtk+-3.0 webkit2gtk-4.0`
LDFLAGS = `pkg-config --libs gtk+-3.0 webkit2gtk-4.0`

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

TARGET = build/kamilBrowser-linux-0.0.1

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) -Iinclude $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -rf build

run:
	$(MAKE) clean
	$(MAKE)
	./$(TARGET)
