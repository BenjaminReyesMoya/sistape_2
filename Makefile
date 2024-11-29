# Variables
CC = gcc            # Compilador
CFLAGS = -Wall -O2  # Opciones de compilación
SRC = $(wildcard Combinacion_*.c) # Todos los archivos fuente Combinacion_i.c
OBJ = $(SRC:.c=.o)  # Archivos objeto
EXE = $(SRC:.c=)    # Ejecutables

# Regla principal
all: $(EXE)

# Regla para crear cada ejecutable
%: %.c
	$(CC) $(CFLAGS) -o $@ $<

# Limpieza
clean:
	rm -f $(OBJ) $(EXE)
