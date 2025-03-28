all: main.c
	gcc main.c -o lvl-edit -std=c99 -Wall -lraylib -I . -lm
