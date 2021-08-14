CFLAGS = -O0 -g

all: lrparser.tab.cpp lex.yy.cpp ast.cpp genIR.cpp IRTypes.cpp main.cpp
	gcc -o a $(CFLAGS) lrparser.tab.cpp lex.yy.cpp ast.cpp genIR.cpp IRTypes.cpp main.cpp opt.cpp --std=c++17 -lstdc++


lrparser.tab.cpp : lrparser.y
	bison -o lrparser.tab.cpp -d lrparser.y

lex.yy.cpp : lrlex.l
	flex -o lex.yy.cpp lrlex.l
	


clean:
	$(RM) *.o *.exe lrparser.tab.cpp c
