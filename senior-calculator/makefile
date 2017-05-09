cc=gcc
all:
	make senior-calculator
senior-calculator: src/*
	bison -o obj/$@.tab.c -d src/$@.y
	flex -o obj/$@.lex.c src/$@.l
	cc -o bin/$@ obj/*.c src/*.c -lm -lfl
clean:
	rm bin/*
	rm obj/*