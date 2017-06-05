all:
	make yascc
scc:
	bison -o yascc.tab.c -d yascc.y
	flex -o yascc.lex.c yascc.l
	cc -o yascc yascc.lex.c yascc.tab.c yascc.c
clean:
	rm *.tab.*
	rm *.lex.*
	rm yascc