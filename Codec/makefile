CC = gcc
CFLAGS = -Wall -Wextra -fPIC -Iinclude
LDFLAGS = -shared
RM = rm -f

# Nom de la librairie
TARGET_LIB = libdif.so

# Sources
SRCS = src/dif.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET_LIB)

$(TARGET_LIB): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET_LIB)