all:
	make acllh
acllh: acllh.l
	flex -o acllh.lex.c acllh.l
	cc -o acllh acllh.lex.c -lfl
clean:
	rm acllh.lex.c
	rm acllh