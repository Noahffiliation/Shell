all:
	gcc -o tush tush.c

coverage:
	gcc --coverage -g -o tush tush.c

clean:
	rm -f tush tests *.gcno *.gcda *.gcov
