#include <stdio.h>

int main(int argc, char **argv, char **envp) {
	char **c = envp;
	while (*c) {
		puts(*c);
		c++;
	}
	return 0;
}

