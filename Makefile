# GNU make

DEBUG = -g
CFLAGS = -ansi -Wall -pedantic -I ${IDIR} ${DEBUG}
IDIR = include/
MAKEDEPFLAG = -M

DLX = dlx.o
DLX_DIR = dlx
SUDOKU = sudoku.o
SUDOKU_DIR = sudoku
MATRIX = matrix.o
MATRIX_DIR = matrix

ssudoku: ${DLX} ${SUDOKU} main.o
	${CC} ${CFLAGS} -o $@ $^

test: ${DLX} ${MATRIX} test.o
	${CC} ${CFLAGS} -o $@ $^

OBJ = ${DLX} ${SUDOKU} ${MATRIX} main.o test.o

main.o: CFLAGS += -D _POSIX_C_SOURCE=200809

${DLX}: %.o: ${DLX_DIR}/%.c
	${CC} ${CFLAGS} -c $<

${SUDOKU}: %.o: ${SUDOKU_DIR}/%.c
	${CC} ${CFLAGS} -c $<

${MATRIX}: %.o: ${MATRIX_DIR}/%.c
	${CC} ${CFLAGS} -c $<

%.o: %.c
	${CC} ${CFLAGS} -c $<

depend: */*.c *.c
	${CC} ${CFLAGS} ${MAKEDEPFLAG} $^ > $@

clean: 
	-rm -f ${OBJ} test ssudoku 

.PHONY: clean

include depend
