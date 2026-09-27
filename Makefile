CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = complac
SRCS = main.c lex.c token.c parser.c symtab.c diag.c opt.c log.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET)

.PHONY: all clean
