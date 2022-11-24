#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/neutrino.h>
#include <unistd.h>

#include "constants.h"

#define NUM_PIDS 5


int main(void) {
	printf("Hello World!!!\n"); /* prints Hello World!!! */

	pid_t rars_pids[NUM_PIDS];
	int temp_sensor_fd[2];// 0=RD 1=WR

	/* Create the pipes representing sensors*/
	if(pipe(temp_sensor_fd) == -1){
		perror("pipe(temp_sensor_fd) failed.");
		exit(EXIT_FAILURE);
	}

	/*****************************************************************************
	 * Create the mock drivers for the sensors and pass the read end of the pipe
	 *****************************************************************************/

	// Temp mock data to read till the envSimulator is in place
	FILE *write_file = fdopen(temp_sensor_fd[1], "w");
	fprintf(write_file, "24.3 25 24 28 26.3 30.0 24.3 25 24 28 26.3 30.0 24.3 25 24 28 26.3 30.0 24.3 25 24 28 26.3 31.0");

	/* Map the read end of pipe to stdin (could just not and it would keep same fd but this is fine for now)*/
	char *ts_args[] = {"/tmp/temperature_sensor", NULL};
	int ts_fd_map[] = {temp_sensor_fd[0]};

	/*Create temp sensor proc*/
	rars_pids[0] = spawn("/tmp/temperature_sensor", 1, ts_fd_map, NULL, ts_args, NULL);
	if(rars_pids[0] == -1){
		perror("Failed to spawn temp sensor");
		exit(EXIT_FAILURE);
	}


	/*****************************************************************************
	 * Create the temp actuators
	 *****************************************************************************/
	char *ac_args[] = {"/tmp/air_conditioner_actuator", NULL};
	rars_pids[1] = spawn("/tmp/air_conditioner_actuator", 0, NULL, NULL, ac_args, NULL);
	if(rars_pids[1] == -1){
		perror("Failed to spawn A/C actuator");
		exit(EXIT_FAILURE);
	}

	char *heater_args[] = {"/tmp/heater_actuator", NULL};
	rars_pids[2] = spawn("/tmp/heater_actuator", 0, NULL, NULL, heater_args, NULL);
	if(rars_pids[2] == -1){
		perror("Failed to spawn heater actuator");
		exit(EXIT_FAILURE);
	}

	char *temp_act_args[] = {"/tmp/temperature_actuator", NULL};
	rars_pids[3] = spawn("/tmp/temperature_actuator", 0, NULL, NULL, temp_act_args, NULL);
	if(rars_pids[3] == -1){
		perror("Failed to spawn temp actuator");
		exit(EXIT_FAILURE);
	}

	/*****************************************************************************
	 * Create the temp manager clients for the sensors
	 *****************************************************************************/
	char *tm_args[] = {"/tmp/temperature_manager", TEMPERATURE_SENSOR_SERVER, NULL};
	rars_pids[4] = spawn("/tmp/temperature_manager", 0, NULL, NULL, tm_args, NULL);
	if(rars_pids[4] == -1){
		perror("Failed to spawn temp manager");
		exit(EXIT_FAILURE);
	}



	/* Exec() the Display Process
	 *
	 * The display Process will take over execution from here but pass it the rars_pids so it can kill everything
	 * */
	char *args[NUM_PIDS+2];
	/* Convert all the pids to strings */
	for (int i=0; i<NUM_PIDS; i++){
		args[i+1] = malloc(10*sizeof(char));
		sprintf(args[i+1], "%d", rars_pids[i]);
		printf("Converted %d to %s\n", rars_pids[i], args[i+1]);
	}
	args[0] = "/tmp/display";
	args[NUM_PIDS+1] = NULL;
	if (execv("/tmp/display", args) == -1){
		perror("Could not start display. Shutting down RARS");
		exit(EXIT_FAILURE);
	}


	return EXIT_SUCCESS;
}
