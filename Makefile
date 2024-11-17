all:
	gcc -std=c99 -pedantic -Wall -Wextra src/my_db/*.c -D_POSIX_C_SOURCE=200809
