/*
 * temperature_sensor.c
 *
 *	Multi-threaded process that pulls data from the read end of the pipe queue and stores the data point.
 *	Another thread waits on Client messages for data and returns the data
 *
 *  Created on: Nov. 18, 2022
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

/*
 * Constants
 */
#define BUFFER_SIZE 16


/*
 *  our global variables.
 */
volatile int state; // which state we are in


/*
 *  our mutex variable for the temp data
 */

pthread_mutex_t temp_data_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Threads functions*/
void *runServer();
void *readData();

/*
 * Structs for passing data to thread functions
 */
struct thread_args {
	char* temp_data;
	FILE *log_file;
};

/*
 * Struct for get messages
 */
typedef union
{
	uint16_t type;
	struct _pulse pulse;
	get_snsr_data__msg_t get_data;
} recv_buf_t;



/**
 * Temperature sensor script
 *
 * Runs two threads. One reads data from STDIN which has been mapped to the RD end of a pipe.
 * The other runs a server and handles requests for the most recent data
 *
 *
 */
int main(void) {
	FILE *log_file1;
	log_file1 = fopen("/tmp/sensor_reader.log", "w");
	fprintf(log_file1, "Starting temp sensor\n");

	FILE *log_file2;
	log_file2 = fopen("/tmp/sensor_server.log", "w");
	fprintf(log_file2, "Starting temp sensor\n");


	char temperature_data[BUFFER_SIZE];
	pthread_t read_thread, server_thread;

	/*Spawn readData and runServer threads*/
	struct thread_args server_args;
	server_args.temp_data = temperature_data;
	server_args.log_file = log_file2;
	pthread_create(&server_thread, NULL, runServer, &server_args);

	struct thread_args t_args;
	t_args.temp_data = temperature_data;
	t_args.log_file = log_file1;
	pthread_create(&read_thread, NULL, readData, &t_args);

	pthread_join(read_thread, NULL);
	pthread_join(server_thread, NULL);

	/*wait on the threads*/
}

void *readData(void *args){

	/*Pull args from the thread_args*/
	struct thread_args *t_args = (struct thread_args *) args;
	FILE *log_file = t_args->log_file;
	char *temp_data = t_args->temp_data;

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
		pthread_mutex_unlock(&temp_data_mutex);

		sleep(3);
	}

	return 0;
}

void *runServer(void *args){
	/*Pull args from the thread_args*/
	struct thread_args *t_args = (struct thread_args *) args;
	FILE *log_file = t_args->log_file;
	char *temp_data = t_args->temp_data;

	/*Server args*/
	int 			rcvid;
	name_attach_t 	*attach;
	recv_buf_t 		rbuf;


	// register our name for a channel
	attach = name_attach(NULL, TEMPERATURE_SENSOR_SERVER, 0);
	if (attach == NULL){
		fprintf(log_file, "Could not start server");
		exit(EXIT_FAILURE);
	}
	fprintf(log_file, "Starting Server\n");


	while (1) {
		//receive message
		rcvid = MsgReceive(attach->chid, &rbuf, sizeof(rbuf), NULL);

		 if (0 == rcvid) {
			//received a pulse
			 switch(rbuf.pulse.code){
			 case _PULSE_CODE_DISCONNECT:
				printf("Received disconnect from pulse\n");
				if (-1 == ConnectDetach(rbuf.pulse.scoid)) {
					perror("ConnectDetach");
				}
				break;
			 default:
				 printf("Unknown pulse received. Code: %d\n", rbuf.pulse.code);
			 }

		} else {
			// we got a message, check its type and process the msg based on its type
			switch(rbuf.type){
			case GET_DATA:
				// Get the mutex and return the temp data
				pthread_mutex_lock(&temp_data_mutex);

				printf("Return data: %s", temp_data);

				/* Build the response */
				resp_snsr_data_msg_t resp;
				resp.data = atof(temp_data);

				fprintf(log_file, "Sending %s converted to %f", temp_data, resp.data);

				/* Send data back */
				MsgReply(rcvid, EOK, &resp, sizeof(resp));

				/* Free the mutex */
				pthread_mutex_unlock(&temp_data_mutex);
				break;
			default:
				printf("Unknown message type\n");
				MsgReply(rcvid, EOK, "OK", sizeof("OK"));
			}
		}
	}

	//TODO: Clean up with name_dettatch in the sigin handler
	return 0;

}




