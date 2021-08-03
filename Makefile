CFLAGS = -O0 -g

all: lrparser.tab.cpp lex.yy.cpp ast.cpp genIR.cpp IRTypes.cpp compiler.cpp
	g++ --std=c++17 -o a $(CFLAGS) compiler.cpp lrparser.tab.cpp lex.yy.cpp ast.cpp genIR.cpp IRTypes.cpp Utilities.cpp Flow.cpp InstructionUtilities.cpp


lrparser.tab.cpp : lrparser.y
	bison -o lrparser.tab.cpp -d lrparser.y

lex.yy.cpp : lrlex.l
	flex -o lex.yy.cpp lrlex.l
	


clean:
	$(RM) *.o *.exe lrparser.tab.cpp c
