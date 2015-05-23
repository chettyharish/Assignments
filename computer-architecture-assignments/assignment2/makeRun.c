#include <string.h>
#include <stdio.h>

int main(int argc, char **argv) {
	system("gcc -o assemble a.c");
	char str[100] = ("./assemble ");
	strcat(str, argv[1]);
	strcat(str, " program.mc");
	system(str);
	system("gcc -o simulate lc2sim.c");
	system("./simulate program.mc");
//	system("./simulate program.mc > output.txt");
//	system("cat output.txt");
	return 0;
}
