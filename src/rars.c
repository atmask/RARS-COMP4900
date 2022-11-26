#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/neutrino.h>
#include <unistd.h>
#include <sys/dispatch.h>
#include <string.h>

#include "constants.h"

#define NUM_PIDS 5

/********************************
 * Signals
 *******************************/
/*Atomic var to track running state*/
volatile sig_atomic_t running = 1;
void cleanup_and_exit(int);


int main(void) {
	/* Register signal handle to receive user INT*/
	signal(SIGINT, cleanup_and_exit);
	signal(SIGTERM, cleanup_and_exit);
	signal(SIGKILL, cleanup_and_exit);


	/*****************************************************************************
	 * Start the Display server so the manager can connect
	 *****************************************************************************/
	name_attach_t *attach;
	struct _pulse msg;
	int rcvid;

	printf("**************************************************************\n");
	printf("\tWELCOME TO THE REAL-TIME AGRICULTURAL SYSTEM (RARS)\n");
	printf("**************************************************************\n\n\n");


	attach = name_attach(NULL, DISPLAY_SERVER, 0);
	if (attach == NULL){
		printf("Could not start display\n");
		exit(EXIT_FAILURE);
	}
	/*****************************************************************************
	 * END: Display start
	 *****************************************************************************/


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
	char s[10];
	sprintf(s, "%d", temp_sensor_fd[1]);
	/* Map the write end of pipe to stdin (could just not and it would keep same fd but this is fine for now)*/
	char *es_args[] = {"/tmp/temperature_sensor", s, NULL};

	/*Create environment simulator proc*/
	rars_pids[0] = spawn("/tmp/environment_simulator", 0, NULL, NULL, es_args, NULL);
	if(rars_pids[0] == -1){
		perror("Failed to spawn environment simulator");
		exit(EXIT_FAILURE);
	}

	usleep(30);
	/* Map the read end of pipe to stdin (could just not and it would keep same fd but this is fine for now)*/
	char *ts_args[] = {"/tmp/temperature_sensor", NULL};
	int ts_fd_map[] = {temp_sensor_fd[0]};

	/*Create temp sensor proc*/
	rars_pids[1] = spawn("/tmp/temperature_sensor", 1, ts_fd_map, NULL, ts_args, NULL);
	if(rars_pids[1] == -1){
		perror("Failed to spawn temp sensor");
		exit(EXIT_FAILURE);
	}


	/*****************************************************************************
	 * Create the temp actuators
	 *****************************************************************************/
	char *ac_args[] = {"/tmp/air_conditioner_actuator", NULL};
	rars_pids[2] = spawn("/tmp/air_conditioner_actuator", 0, NULL, NULL, ac_args, NULL);
	if(rars_pids[2] == -1){
		perror("Failed to spawn A/C actuator");
		exit(EXIT_FAILURE);
	}

	char *heater_args[] = {"/tmp/heater_actuator", NULL};
	rars_pids[3] = spawn("/tmp/heater_actuator", 0, NULL, NULL, heater_args, NULL);
	if(rars_pids[3] == -1){
		perror("Failed to spawn heater actuator");
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
//	char *args[NUM_PIDS+2];
//	/* Convert all the pids to strings */
//	for (int i=0; i<NUM_PIDS; i++){
//		args[i+1] = malloc(10*sizeof(char));
//		sprintf(args[i+1], "%d", rars_pids[i]);
//		printf("Converted %d to %s\n", rars_pids[i], args[i+1]);
//	}
//	args[0] = "/tmp/display";
//	args[NUM_PIDS+1] = NULL;
//	if (execv("/tmp/display", args) == -1){
//		perror("Could not start display. Shutting down RARS");
//		exit(EXIT_FAILURE);
//	}


	/*****************************************************************************
	 * Start the Display Logics
	 *****************************************************************************/

	//Store the data for the print
	float temp;
	char ac_state[8];
	char heater_state[8];
	strcpy(ac_state, "OFF");
	strcpy(heater_state, "OFF");

	// Create a timer to periodically print the display
	struct sigevent sigevent;
	struct itimerspec itime;
	timer_t timerID;
	int event_coid;
	event_coid = ConnectAttach(0, 0, attach->chid, _NTO_SIDE_CHANNEL, 0);
	SIGEV_PULSE_INIT(&sigevent, event_coid, SIGEV_PULSE_PRIO_INHERIT, TIMER_PULSE_CODE, 0);
	timer_create(CLOCK_REALTIME, &sigevent, &timerID);
	itime.it_value.tv_sec = 2;
	itime.it_value.tv_nsec = 0;
	itime.it_interval.tv_sec = 2;
	itime.it_interval.tv_nsec = 0;
	timer_settime(timerID, 0, &itime, NULL);

	while (running) {
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), 0);
		 if (0 == rcvid) {
			 switch(msg.code){
			 case _PULSE_CODE_DISCONNECT:
				printf("Received disconnect from pulse\n");
				if (-1 == ConnectDetach(msg.scoid)) {
					perror("ConnectDetach");
				}
				break;
			 case TIMER_PULSE_CODE:
				 printf("******************************\n\tSENSORS\n******************************\n");
				 printf("Temperature Sensor: %.2f\n\n", temp);
				 printf("******************************\n\tACTUATORS\n******************************\n");
				 printf("A/C Unit:\t\t%s\n", ac_state);
				 printf("Heating Unit:\t\t%s\n", heater_state);
				 printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");

				 break;
			 case KILL_ALL:
			 	 printf("Pulse received:\n[DISPLAY] Killing all processes\n");
			 	 break;
			 case TEMP_DATA:
				 //printf("Pulse received:\n[DISPLAY] Temperature Sensor: %d\n", msg.value);
				 temp = msg.value.sival_int;
				 break;
			 case TEMP_AC:
				 if(msg.value.sival_int == ON){
					strcpy(ac_state, "ON");
					//printf("Pulse received:\n[DISPLAY] AC has turned on\n");
				 }else if(msg.value.sival_int == OFF){
					strcpy(ac_state, "OFF");
					//printf("Pulse received:\n[DISPLAY] AC has turned off\n");
				 }
			 	break;
			 case TEMP_HEATER:
				 //heater_state = sg.value.sival_int;
				 if(msg.value.sival_int == ON){
					strcpy(heater_state, "ON");
					//printf("Pulse received:\n[DISPLAY] Heater has turned on\n");
				 }else if(msg.value.sival_int == OFF){
					strcpy(heater_state, "OFF");
					// printf("Pulse received:\n[DISPLAY] Heater has turned off\n");
				 }
			 	 break;
			 default:
				 printf("Unexpected pulse code: %d", msg.code);
				 break;
			 }
		} else {
			printf("Unexpected msg. Replying OK\n");
			MsgReply(rcvid, EOK, "OK", sizeof("OK"));
		}
	}

	/* Kill the running procs generated for RARS and exit gracefully */
	//ignore the bin name but convert all the pids to int and kill with with SIGTERM
	for(int i=0; i<NUM_PIDS; i++){
		printf("Killing pid: %d\n", rars_pids[i]);
		kill(rars_pids[i], SIGTERM);
	}


	return EXIT_SUCCESS;
}


void cleanup_and_exit(int sig_no){
	printf("Interrupt: Exiting\n");
	running = 0;
}

