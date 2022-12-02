/*
 * environment_actuator.c
 *
 *	Simulation process that feeds data to the sensros and is affected by the actuators
 *
 *  Created on: Nov. 25, 2022
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <process.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>

#include "constants.h"
#include "utilities.h"


/*ACTUATOR TYPES*/
#define HEATER 0
#define AIR_CONDITIONER 1
#define HUMIDIFIER 2
#define DEHUMIDIFIER 3

#define NUM_ACTUATORS 4

/*DATA OUTPUT TYPES*/
#define TEMPERATURE 0
#define HUMIDITY 1

#define NUM_OUTPUTS 2

/*ACTUATOR EFFECTS ON LINKED VARIABLE*/
#define RAND 0 //Actuator off - variable linked to actuator will vary randomly
#define UP 1
#define DOWN 2

/*
 * Struct for cmd messages
 */
typedef union
{
	uint16_t type;
	struct _pulse pulse;
} recv_pulse_t;

/*
 * Structs for passing data to write thread
 */
struct thread_args {
	int* fds;
};

/* Track the running state in an atomic var modified by signal handler*/
volatile sig_atomic_t running = 1;

/* mutex for locking output data variance type array */
pthread_mutex_t var_types_mutex = PTHREAD_MUTEX_INITIALIZER;

/* How to change the current the value of different outputs on the next iteration. UP/DOWN/RAND */
int varianceTypes[NUM_OUTPUTS];

/* Functions */
void set_exit_flag(int sig_no);
void *writeData();
void *runServer();
void updateVarianceTypes();

int main(int argc, char *argv[]) {
	pthread_t write_thread, server_thread;
	int pipe_fds[NUM_OUTPUTS];
	struct thread_args writer_args;

	/*Prepare data writer arguments*/
	pipe_fds[TEMPERATURE] = atoi(argv[1]);
	writer_args.fds = pipe_fds;

	/*Spawn threads*/
	pthread_create(&server_thread, NULL, runServer, NULL);
	pthread_create(&write_thread, NULL, writeData, &writer_args);


	/*wait on the threads*/
	pthread_join(write_thread, NULL);
	pthread_join(server_thread, NULL);
}

void *writeData(void* args)
{
	FILE 			*log_file;
	struct thread_args *t_args = (struct thread_args *) args;

	log_file = fopen("/tmp/environment_simulator_writer.log", "w");
	logString(log_file, "Starting environment simulator data writer");

	float outputDataPoints[NUM_OUTPUTS] = {22.5f};
	srand(time(0));

	while(running)
	{
		//Create outputs
		pthread_mutex_lock(&var_types_mutex);
		for(int i = 0; i < NUM_OUTPUTS; ++i)
		{
			float r = (((float)rand()) / (float)RAND_MAX) * 3.0f;
			if(varianceTypes[i] == DOWN)
			{
				r *= -1.0f;
			}
			else if(varianceTypes[i] == RAND)
			{
				if((((float)rand()) / (float)RAND_MAX) > 0.5f)
				{
					r *= -1.0f;
				}
			}
			outputDataPoints[i] += r;
			dprintf(t_args->fds[i], "%f\n",  + outputDataPoints[i]);
			logString(log_file, "Printing %f to pipe %d. Change of %f", outputDataPoints[i], i, r);
		}
		pthread_mutex_unlock(&var_types_mutex);
		sleep(1);
	}
	return EXIT_SUCCESS;
}

void *runServer(void* args)
{
	FILE 			*log_file;
	int 			rcvid;
	name_attach_t 	*attach;
	recv_pulse_t 	rbuf;
	int 			actuatorStates[NUM_ACTUATORS] = {OFF, OFF, OFF, OFF};

	//set up log file
	log_file = fopen("/tmp/environment_simulator_server.log", "w");
	logString(log_file, "Starting environment simulator server");

	// register our name for a channel
	attach = name_attach(NULL, ENVIRONMENT_SIMULATOR_SERVER, 0);
	if (attach == NULL){
		logString(log_file, "Could not start server. %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	logString(log_file, "Starting Server");


	while (running) {
		//receive message
		rcvid = MsgReceive(attach->chid, &rbuf, sizeof(rbuf), NULL);

		 if (0 == rcvid) {
			//received a pulse
			 switch(rbuf.pulse.code){
			 case _PULSE_CODE_DISCONNECT:
				logString(log_file, "Received disconnect from pulse\n");
				if (-1 == ConnectDetach(rbuf.pulse.scoid)) {
					perror("ConnectDetach");
				}
				break;
			 case HEATER_ACTUATOR_CHANGE:
				 actuatorStates[HEATER] = rbuf.pulse.value.sival_int;
				 logString(log_file, "Changing heater actuator to state %d", actuatorStates[HEATER]);
				 break;
			 case AIR_CONDITIONER_ACTUATOR_CHANGE:
				 actuatorStates[AIR_CONDITIONER] = rbuf.pulse.value.sival_int;
				 logString(log_file, "Changing air conditioner actuator to state %d", actuatorStates[AIR_CONDITIONER]);
				 break;
			 case HUMIDIFIER_ACTUATOR_CHANGE:
				 actuatorStates[HUMIDIFIER] = rbuf.pulse.value.sival_int;
				 logString(log_file, "Changing air conditioner actuator to state %d", actuatorStates[HUMIDIFIER]);
				 break;
			 case DEHUMIDIFIER_ACTUATOR_CHANGE:
				 actuatorStates[DEHUMIDIFIER] = rbuf.pulse.value.sival_int;
				 logString(log_file, "Changing air conditioner actuator to state %d", actuatorStates[DEHUMIDIFIER]);
				 break;
			 default:
				 logString(log_file, "Unknown pulse received. Code: %d\n", rbuf.pulse.code);
			 }

		} else {
			// we got a message, but none are supported
			logString(log_file, "Received unknown message type: %d", rbuf.type);
			MsgReply(rcvid, ENOTSUP, "Not supported", sizeof("Not supported"));
		}
		updateVarianceTypes(log_file, actuatorStates);
	}
	fclose(log_file);
	name_detach(attach, 0);
	return EXIT_SUCCESS;
}

void updateVarianceTypes(FILE* log_file, int actuatorStates[])
{
	 pthread_mutex_lock(&var_types_mutex);
	 //Temperature:
	 if(actuatorStates[HEATER] == ON && actuatorStates[AIR_CONDITIONER] == OFF)
	 {
		 logString(log_file, "Changing temperature variation to UP");
		 varianceTypes[TEMPERATURE] = UP;
	 }
	 else if(actuatorStates[HEATER] == OFF && actuatorStates[AIR_CONDITIONER] == ON)
	 {
		 logString(log_file, "Changing temperature variation to DOWN");
		 varianceTypes[TEMPERATURE] = DOWN;
	 }
	 else
	 {
		 logString(log_file, "Changing temperature variation to RANDOM");
		 varianceTypes[TEMPERATURE] = RAND;
	 }

	 //Humidity:
	 if(actuatorStates[HUMIDIFIER] == ON && actuatorStates[DEHUMIDIFIER] == OFF)
	 {
		 logString(log_file, "Changing humidity variation to UP");
		 varianceTypes[HUMIDITY] = UP;
	 }
	 else if(actuatorStates[HUMIDIFIER] == OFF && actuatorStates[DEHUMIDIFIER] == ON)
	 {
		 logString(log_file, "Changing humidity variation to DOWN");
		 varianceTypes[HUMIDITY] = DOWN;
	 }
	 else
	 {
		 logString(log_file, "Changing humidity variation to RANDOM");
		 varianceTypes[HUMIDITY] = RAND;
	 }
	 pthread_mutex_unlock(&var_types_mutex);
}

void set_exit_flag(int sig_no){
	running = 0;
}
