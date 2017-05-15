all:
	make scc
scc:
	bison -o scc.tab.c -d scc.y
	flex -o scc.lex.c scc.l
	cc -o scc scc.lex.c scc.tab.c
clean:
	rm *.tab.*
	rm *.lex.*
	rm scc