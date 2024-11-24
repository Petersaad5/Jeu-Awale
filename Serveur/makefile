CC = gcc
CFLAGS = -w
LDFLAGS = -lm
TARGET = server
SRCS = server2.c awale.c

$(TARGET): $(SRCS)
	$(CC) $(SRCS) $(CFLAGS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)