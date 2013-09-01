all: runCommand

runCommand: runCommand.o
	gcc -o runCommand runCommand.o

runCommand.o: runCommand.c
	gcc -c runCommand.c

clean:
	rm -rf *.o runCommand
