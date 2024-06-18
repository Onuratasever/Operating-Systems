all: clear compile run clean

fileSystemOper: clear compile2 run2 clean
makeFileSystem: clear compile1 run1 clean

clean:
	@rm -f *.out
	@rm -f *.o
	@rm -f makeFileSystem

clear:
	@clear

compile:
	@gcc -o makeFileSystem makeFileSystem.c
compile2:	
	@gcc fileSystemOper.c -o oper
run:
	@./makeFileSystem 0.5 file.txt
run2:
	@./oper file.txt mkdir /usr/ysa/abc