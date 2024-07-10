CC = gcc
CFLAGS = -Wall -Wextra -pedantic
TARGET = scribe
SRC = scribe.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

.PHONY: clean

clean:
	rm -f $(TARGET)
