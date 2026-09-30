CMD = gcc
SRC = enc.c
OBJ = $(SRC:.c=.o)
OUT = enc
WRN = -Wall -Wpedantic -Wextra

all : ${OBJ}
	${CMD} $^ ${WRN} -o ${OUT}
${OBJ} : %.o: %.c
	${CMD} ${WRN} -c $^ -o $@

clean :
	@find . -type f -name '*.o' -delete
	@rm ${OUT}
