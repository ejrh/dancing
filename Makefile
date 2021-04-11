CFLAGS = -Wall $(OPT)

all: queens pentominoes sudoku

SHARED_FILES = dancing.c dancing.h extarray.h
SHARED_C = dancing.c
SHARED_OBJ = dancing.o

queens: $(SHARED_FILES) queens.c
	$(CC) -o $@ $(SHARED_C) queens.c $(CFLAGS)

pentominoes: $(SHARED_FILES) pentominoes.c
	$(CC) -o $@ $(SHARED_C) pentominoes.c $(CFLAGS)

sudoku: $(SHARED_FILES) sudoku.c
	$(CC) -o $@ $(SHARED_C) sudoku.c $(CFLAGS)

%.o: %.c $(HEADERS)
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f $(OBJ)
