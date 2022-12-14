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
#include "utilities.h"

/*
 * Constants
 */
#define BUFFER_SIZE 16


/*
 *  our global variables.
 */
volatile sig_atomic_t running = 1;



/*
 *  our mutex variable for the temp data
 */

pthread_mutex_t temp_data_mutex = PTHREAD_MUTEX_INITIALIZER;

float 			temp_data;

/* Functions*/
void *runServer();
void *readData();

/*
 * Structs for passing data to thread functions
 */
struct thread_args {
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
	log_file1 = fopen("/tmp/temp_sensor_reader.log", "w");
	logString(log_file1, "Starting temp sensor");

	FILE *log_file2;
	log_file2 = fopen("/tmp/temp_sensor_server.log", "w");
	logString(log_file2, "Starting temp sensor");

	pthread_t read_thread, server_thread;


	/*Spawn readData and runServer threads*/
	struct thread_args server_args;
	server_args.log_file = log_file2;
	pthread_create(&server_thread, NULL, runServer, &server_args);

	struct thread_args t_args;
	t_args.log_file = log_file1;
	pthread_create(&read_thread, NULL, readData, &t_args);


	/*wait on the threads*/
	pthread_join(read_thread, NULL);
	//Don't wait on the server thres because it is recieve blocked and won't shut down
	pthread_join(server_thread, NULL);

}

void *readData(void *args){

	/*Pull args from the thread_args*/
	struct thread_args *t_args = (struct thread_args *) args;
	FILE *log_file = t_args->log_file;

	while(running){
		/* Lock the mutex */
		pthread_mutex_lock(&temp_data_mutex);

		/* Scanf to get the temp
		 * N.B. Scanf reads up the whitespace
		 * */
		fscanf(stdin, "%f", &temp_data);
		temp_data = temp_data * 100;
		logString(log_file, "Read data: %f", temp_data);

		pthread_mutex_unlock(&temp_data_mutex);

		sleep(1);
	}

	/* Cleanup */
	fclose(log_file);
	return 0;
}

void *runServer(void *args){
	/*Pull args from the thread_args*/
	struct thread_args *t_args = (struct thread_args *) args;
	FILE *log_file = t_args->log_file;
	/*Server args*/
	int 			rcvid;
	name_attach_t 	*attach;
	recv_buf_t 		rbuf;

	srand(time(NULL));


	// register our name for a channel
	attach = name_attach(NULL, TEMPERATURE_SENSOR_SERVER, 0);
	if (attach == NULL){
		logString(log_file, "Could not start server. %s", strerror(errno));
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
			 case _PULSE_CODE_UNBLOCK:

				 fprintf(log_file, "UNBLOCK RECIEVED\n");
				 fflush(log_file);
				 break;
			 default:
				 logString(log_file, "Unknown pulse received. Code: %d", rbuf.pulse.code);
			 }

		} else {
			// we got a message, check its type and process the msg based on its type
			switch(rbuf.type){
			case GET_DATA:
				// Get the mutex and return the temp data
				pthread_mutex_lock(&temp_data_mutex);
				logString(log_file, "Return data: %f", temp_data);
				/* Build the response */
				resp_snsr_data_msg_t resp;
				resp.data = temp_data;

				/* Free the mutex so it does not get reply blocked*/
				pthread_mutex_unlock(&temp_data_mutex);

				/* Send data back */
				MsgReply(rcvid, EOK, &resp, sizeof(resp));

				break;
			default:
				logString(log_file, "Unknown message type");
				MsgReply(rcvid, EOK, "OK", sizeof("OK"));
			}
		}
	}

	/* Cleanup */
	name_detach(attach, 0);
	fclose(log_file);
	return 0;
}



