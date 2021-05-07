CFLAGS = -Wall -Wno-unused $(OPT) -lpthread

all: queens pentominoes sudoku

SHARED_FILES = dancing.c dancing.h extarray.h segarray.h dancing_threads.c dancing_threads.h basic.c basic.h
SHARED_C = dancing.c dancing_threads.c basic.c
DELETABLE = queens pentominoes sudoku *.o

queens: $(SHARED_FILES) queens.c
	$(CC) -o $@ $(SHARED_C) queens.c $(CFLAGS)

pentominoes: $(SHARED_FILES) pentominoes.c
	$(CC) -o $@ $(SHARED_C) pentominoes.c $(CFLAGS)

sudoku: $(SHARED_FILES) sudoku.c
	$(CC) -o $@ $(SHARED_C) sudoku.c $(CFLAGS)

%.o: %.c $(HEADERS)
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f $(DELETABLE)
