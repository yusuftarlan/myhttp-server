# Program Name
NAME = server

# Compiler
CC = gcc

# Source files
SRC = src/*

# Header files' path
INCLUDE_PATH = include

# Compilation Flags
FLAGS =  -Wall -Wextra -Wpedantic -g -pthread

all: ${NAME}

${NAME}: src/*
	${CC} ${FLAGS} ${SRC} -I${INCLUDE_PATH} -o ${NAME}

run: ${NAME}
	./${NAME}

valgrind: ${NAME}
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -s ./${NAME}