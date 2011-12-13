# GNU make

DEBUG = -g
CFLAGS = -ansi -Wall -pedantic -I ${IDIR} ${DEBUG}
CTAGS = ctags
IDIR = include/
MAKEDEPFLAG = -M

DLX = dlx.o
DLX_DIR = dlx
SUDOKU = sudoku.o sudoku_grid.o
SUDOKU_DIR = sudoku
MATRIX = matrix.o
MATRIX_DIR = matrix
CURSESLIB = curseslib.o
CURSESLIB_DIR = curseslib
NCSUDOKU = ncsudoku.o
NCSUDOKU_DIR = ncsudoku
OBJ = ${DLX} ${SUDOKU} ${MATRIX} ${CURSESLIB} ${NCSUDOKU} \
      main.o test.o sudoku_ui.o 


all: ssudoku ssudoku2

ssudoku: ${DLX} ${SUDOKU} main.o
	${CC} ${CFLAGS} -o $@ $^

ssudoku2: LDFLAGS += -lncurses

ssudoku2: sudoku_ui.o ${NCSUDOKU} ${CURSESLIB} sudoku_grid.o
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $^

test: ${DLX} ${MATRIX} test.o
	${CC} ${CFLAGS} -o $@ $^

main.o: CFLAGS += -D _POSIX_C_SOURCE=200809

${DLX}: %.o: ${DLX_DIR}/%.c
	${CC} ${CFLAGS} -c $<

${SUDOKU}: %.o: ${SUDOKU_DIR}/%.c
	${CC} ${CFLAGS} -c $<

${MATRIX}: %.o: ${MATRIX_DIR}/%.c
	${CC} ${CFLAGS} -c $<

${CURSESLIB}: %.o: ${CURSESLIB_DIR}/%.c
	${CC} ${CFLAGS} -c $<

${NCSUDOKU}: %.o: ${NCSUDOKU_DIR}/%.c
	${CC} ${CFLAGS} -c $<

%.o: %.c
	${CC} ${CFLAGS} -c $<

depend: */*.c *.c
	${CC} ${CFLAGS} ${MAKEDEPFLAG} $^ > $@

tags: */*.c *.c
	${CTAGS} $^

clean: 
	-rm -f ${OBJ} test ssudoku ssudoku2

.PHONY: clean

include depend
