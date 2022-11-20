/*
 * temperature_sensor.c
 *
 *	Multi-threaded process that pulls data from the read end of the pipe queue and stores the data point.
 *	Another thread waits on Client messages for data and returns the data
 *
 *  Created on: Nov. 18, 2022
 *      Author: benmask
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>


#include "constants.h"

/*
 * Constants
 */
#define BUFFER_SIZE 16


/*
 *  our global variables.
 */
volatile int state; // which state we are in


/*
 *  our mutex and condition variable
 */

pthread_mutex_t temp_data_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


/* Threads functions*/
void runServer();
void readData();

/*
 * Structs for passing data to thread functions
 */
struct read_data_args {
	char* temp_data;
	FILE *log_file;
};


/**
 * Temperature sensor script
 *
 * Runs two threads. One reads data from STDIN which has been mapped to the RD end of a pipe.
 * The other runs a server and handles requests for the most recent data
 *
 *
 */
int main(void) {
	FILE *log_file;
	log_file = fopen("/tmp/sensor.log", "w");
	fprintf(log_file, "Starting temp sensor\n");


	char temperature_data[BUFFER_SIZE];
	pthread_t read_thread;

	/*Spawn readData thread*/
	struct read_data_args rd_args;
	rd_args.temp_data = temperature_data;
	rd_args.log_file = log_file;
	pthread_create(NULL, NULL, readData, &rd_args);

//	readData(temperature_data, log_file);

	/*Spawn the runServer thread*/

	/*wait on the threads*/
}

void readData(void *args){
	struct read_data_args *thread_args = (struct read_data_args *) args;
	FILE *log_file = thread_args->log_file;
	char *temp_data = thread_args->temp_data;

	while(1){
		fprintf(log_file, "get mutex\n");
		/* Lock the mutex */
		pthread_mutex_lock(&temp_data_mutex);

		/* Scanf to get the temp
		 * N.B. Scanf reads up the whitespace
		 * */
		fscanf(stdin, "%s", temp_data);

		fprintf(log_file, "Read data: %s\n", temp_data);

		if(strcmp(temp_data, "31.0") == 0){
			fprintf(log_file, "Done reading\n");
			break;
		}
		//sleep(2);
		pthread_mutex_unlock(&temp_data_mutex);



	}
}

