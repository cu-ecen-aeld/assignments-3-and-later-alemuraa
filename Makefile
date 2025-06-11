# Nome dell'eseguibile
TARGET = writer

# Sorgenti
SRCS = finder-app/writer.c

# Oggetti (sostituisce .c con .o)
OBJS = $(SRCS:.c=.o)

# Prefisso compilatore (vuoto di default, o es. aarch64-none-linux-gnu- per cross)
CROSS_COMPILE ?= 

# Compilatore e flags
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Wextra -Werror -g

.PHONY: all clean

# Target predefinito
all: $(TARGET)

# Regola per costruire l'eseguibile
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Regola per compilare ogni file .c in .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Target pulito: elimina eseguibile e file oggetto
clean:
	rm -f $(TARGET) $(OBJS)

