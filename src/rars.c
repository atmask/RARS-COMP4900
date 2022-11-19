#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/neutrino.h>
#include <unistd.h>

int main(void) {
	puts("Hello World!!!"); /* prints Hello World!!! */

	//arguments for spawned children
	char                *args[] = {};
	//int                 status;
	pid_t               pid;
	struct inheritance  inherit;
	inherit.flags = 0;

	if ((pid = spawn("TempManager", 0, NULL, &inherit, args, environ)) == -1)
		    perror("spawn() failed");



	return EXIT_SUCCESS;
}
