CFLAGS = -Wall -Werror $(OPT)

all: queens pentominoes

SHARED_FILES = dancing.c dancing.h
SHARED_C = dancing.c
SHARED_OBJ = dancing.o

queens: $(SHARED_FILES) queens.c
	$(CC) -o $@ $(SHARED_C) queens.c $(CFLAGS)

pentominoes: $(SHARED_FILES) pentominoes.c
	$(CC) -o $@ $(SHARED_C) pentominoes.c $(CFLAGS)

%.o: %.c $(HEADERS)
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f $(OBJ)
