CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -Iinc
SRC     = src/main.c src/encode.c src/decode.c
OBJ     = $(SRC:.c=.o)
TARGET  = lsb_steg

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Encode samples/secret.txt into samples/beautiful.bmp, decode it again
# and check that the recovered file is byte-for-byte identical.
test: $(TARGET)
	sh tests/roundtrip.sh

clean:
	rm -f $(OBJ) $(TARGET) tests/*.tmp

.PHONY: all test clean
