#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
	int i, catid, sortid, clid, wstatus, p[2];
	char *clname;
	
	if (pipe(p) == -1) {
		perror("pipe");
		return EXIT_FAILURE;
	}
	
	catid = fork();
	if (catid == 0) {
		// set up arguments
		// (note: in this scenario, we could have modified argv and passed it to execv)
		char **catargs = malloc(sizeof(char *) * (argc + 1));

		catargs[0] = "cat";
		for (i = 1; i < argc; i++) catargs[i] = argv[i];
		catargs[argc] = NULL;

		// child process writes to pipe
		dup2(p[1], STDOUT_FILENO);
		
		// close excess file descriptors
		close(p[0]);
		close(p[1]);
		
		execvp("cat", catargs);
			// must use execv, because the number of arguments is dynamic
		
		perror("cat");
		return EXIT_FAILURE;
	}
	
	sortid = fork();
	if (sortid == 0) {
	
		//child process reads from pipe
		dup2(p[0], STDIN_FILENO);
		
		// close excess file descriptors
		close(p[0]);
		close(p[1]);
		
		execlp("sort", "sort", NULL);
			// can use execl, because the number of arguments is static
	
		perror("sort");
		return EXIT_FAILURE;
	}
	
	// close excess file descriptors
	close(p[0]);
	close(p[1]);  // what happens if we move this after the wait loop?
	
	// wait for child processes to end
	// (is the order always deterministic? why or why not?)
	for (i = 0; i < 2; i++) {
		clid = wait(&wstatus);
		clname = clid == catid ? "cat" : "sort";
		if (WIFEXITED(wstatus)) {
			printf("%s exited with status %d\n", clname, WEXITSTATUS(wstatus));
		} else {
			printf("%s exited abnormally\n", clname);
		}
	}

	return EXIT_SUCCESS;

}
