CC       = gcc
CFLAGS   = -std=c23 -pedantic -Wall -Wextra -Wformat-security -Wduplicated-cond \
           -Wfloat-equal -Wshadow -Wconversion -Wjump-misses-init \
           -Wlogical-not-parentheses -Wnull-dereference -Wvla -Werror \
           -fstack-protector-strong -fsanitize=undefined -fno-sanitize-recover \
           -g -fno-omit-frame-pointer -O1

# Konfiguracja kompilacji
TARGET   = rstack
SRCS     = main.c
OBJS     = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

valgrind: $(TARGET)
	valgrind --leak-check=full -q --error-exitcode=1 ./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean valgrind
