# ==============================================================================
#             Makefile for Topological Intrusion Detection System (TIDS)
# ==============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 $(shell pkg-config --cflags gtk+-3.0)
LIBS = $(shell pkg-config --libs gtk+-3.0) -lm

# Executable name
TARGET = tids_analyzer

# Source and Object files
SRCS = main.c tda.c gui.c
OBJS = $(SRCS:.c=.o)

# Default rule
all: $(TARGET)

# Compile executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

# Compile C source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean temporary object files and executable
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
