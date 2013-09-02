all: runCommand shell

runCommand: runCommand.o
	gcc -o runCommand runCommand.o

runCommand.o: runCommand.c
	gcc -c runCommand.c

shell: shell.o
	gcc -o shell shell.o

shell.o: shell.c
	gcc -c shell.c

clean:
	rm -rf *.o runCommand shell
