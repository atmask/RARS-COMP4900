#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/neutrino.h>
#include <unistd.h>

int main(void) {
	printf("Hello World!!!\n"); /* prints Hello World!!! */

	int temp_sensor_fd[2];// 0=RD 1=WR

	/* Create the pipes representing sensors*/
	if(pipe(temp_sensor_fd) == -1){
		perror("pipe(temp_sensor_fd) failed.");
		exit(EXIT_FAILURE);
	}


	/*Create the mock drivers for the sensors and pass the read end of the pipe*/

	// Temp mock data to read till the envSimulator is in place
	FILE *write_file = fdopen(temp_sensor_fd[1], "w");
	fprintf(write_file, "24.3 25 24 28 26.3 30.0 24.3 25 24 28 26.3 30.0 24.3 25 24 28 26.3 30.0 24.3 25 24 28 26.3 31.0");

	char *ts_args[] = {"/tmp/temperature_sensor", NULL};
	int ts_fd_map[] = {temp_sensor_fd[0]};
	if(spawn("/tmp/temperature_sensor", 1, ts_fd_map, NULL, ts_args, NULL) == -1){
		perror("Failed to spawn temp sensor");
		exit(EXIT_FAILURE);
	}
//
//
//	//arguments for spawned children
//	char                *args[] = {};
//	//int                 status;
//	pid_t               pid;
//	struct inheritance  inherit;
//	inherit.flags = 0;
//
//	if ((pid = spawn("TempManager", 0, NULL, &inherit, args, environ)) == -1)
//		    perror("spawn() failed");
//
//
//



	/* Exec() the Display Process
	 *
	 * The display Process will take over execution from here
	 * */


	return EXIT_SUCCESS;
}
