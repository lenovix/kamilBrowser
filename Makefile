CC = gcc
PKGS = gtk+-3.0 webkit2gtk-4.0 json-glib-1.0
CFLAGS = -Wall -g `pkg-config --cflags $(PKGS)`
LDFLAGS = `pkg-config --libs $(PKGS)`

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

TARGET = build/kamilBrowser-linux-0.0.1
BOOKMARKS_FILE = build/bookmarks.json

all: $(BOOKMARKS_FILE) $(TARGET)

$(BOOKMARKS_FILE):
	echo "[]" > $(BOOKMARKS_FILE)

$(TARGET): $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) -Iinclude $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(BOOKMARKS_FILE)

run:
	$(MAKE) clean
	$(MAKE)
	./$(TARGET)
