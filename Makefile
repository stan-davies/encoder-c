CMD = gcc
SRC = control.c enc.c dec.c
OBJ = $(SRC:.c=.o)
OUT = dbm
WRN = -Wall -Wpedantic -Wextra

all : ${OBJ}
	${CMD} $^ ${WRN} -o ${OUT}
${OBJ} : %.o: %.c
	${CMD} ${WRN} -c $^ -o $@

clean :
	@find . -type f -name '*.o' -delete
	@rm ${OUT}
