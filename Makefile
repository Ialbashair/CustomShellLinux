CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11
TARGET  = mysh

$(TARGET): Main.c
	$(CC) $(CFLAGS) -o $(TARGET) Main.c

clean:
	rm -f $(TARGET)

.PHONY: clean
