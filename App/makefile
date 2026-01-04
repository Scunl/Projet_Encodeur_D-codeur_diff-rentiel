CC = gcc
CFLAGS = -Wall -Wextra -O2 -I../Codec/include
# On lie avec la librairie 'dif' (-ldif) située dans ../Codec (-L)
# RPATH permet à l'exécutable de trouver le .so sans changer LD_LIBRARY_PATH
LDFLAGS = -L../Codec -ldif -Wl,-rpath='$$ORIGIN/../Codec'
RM = rm -f

TARGET = difcodec

SRCS = src/main.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET)