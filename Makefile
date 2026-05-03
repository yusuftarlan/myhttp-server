server: src/*
	gcc -Wall -Werror -Wextra src/* -Iinclude -o server -pthread