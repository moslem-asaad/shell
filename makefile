all: run

run: task3.o LineParser.o
	gcc -m32 -g -Wall -o run task3.o	LineParser.o
	
task3.o: task3.c 
	gcc -g -Wall -m32  -c -o task3.o task3.c 

LineParser.o: LineParser.c
	gcc -g -Wall -m32  -c -o LineParser.o LineParser.c 

#tell make that "clean" is not a file name!
.PHONY: clean

#Clean the build directory
clean: 
	rm -f */.o run
