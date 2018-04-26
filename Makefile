SRC_OBJ=curllearning.o
SRC_BIN=curllearning
SRC_LIB= -ljsoncpp -lcurl 
CC=g++
${SRC_BIN} : ${SRC_OBJ}
	${CC} -o $@ $^ ${SRC_LIB}
	${RM} ${SRC_OBJ}
clean:
	${RM}     ${SRC_OBJ} ${SRC_BIN}