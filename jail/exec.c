#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>


char **loadEnv(const char *fname) {
	
	FILE *f = fopen(fname, "r");
	if (f == NULL) return NULL;
	char buffer[65536];
	char **options = (char **)calloc(10000,sizeof(char *));
	char *s;
	int count = 0;
	while ((s = fgets(buffer,sizeof(buffer), f)) != NULL && count < 10000) {
		char *x = strndup(s,strlen(s)-1);
		if (*x == 0) continue;
		options[count++]  = x;
	}
	options[count] = 0;
	return options;
}

int main(int argc, char **argv, char **envp) {
	if (argc < 3) {
		fprintf(stderr, "need arguments: %s -D <workdir> -E <env> <program> <args>\n", argv[0]);
		return 1;
	}

	int idx = 1;
	while (idx<argc && argv[idx][0] == '-') {
		const char *p = argv[idx++];
		if (p[2] == 0) switch(p[1]) {
			case 'D': if (idx == argc) break;
				  if (chdir (argv[idx++])) {
					int err = errno;
					fprintf(stderr, "error change dir: %s - %d\n", argv[idx-1], err);
					return err;
				  }break;
			case 'E': if (idx == argc) break;
				   envp = loadEnv(argv[idx++]);
				   if (envp == NULL) {
					int err = errno;
					fprintf(stderr, "failed to load env: %s - %d\n", argv[idx-1], err);
					return err;
				   }break;
			default:
				fprintf(stderr, "invalid option %s\n",p);
				return 1;
			

		} else {
			fprintf(stderr, "invalid option %s\n",p);
			return 1;
		}
			
			
	}

	if (execve(argv[idx], argv+idx, envp)) {
		int err = errno;
		fprintf(stderr, "error exec : %s - %d\n", argv[idx], err);
		return err;		
	}
	return 0;
}
