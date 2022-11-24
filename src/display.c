/*
 * display.c
 *
 *  Created on: Nov 21, 2022
 *      Author: Everyone
 */
#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/neutrino.h>
#include <unistd.h>
#include <sys/dispatch.h>


#include "constants.h"

/*Atomic var to track running state*/
volatile sig_atomic_t running = 1;

void cleanup_and_exit(int);

//using printf for outputs for now
int main(int argc, char **argv){

	printf("HERE");
	/* Register signal handle to receive user INT*/
	signal(SIGINT, cleanup_and_exit);
	signal(SIGTERM, cleanup_and_exit);
	signal(SIGKILL, cleanup_and_exit);


	printf("HERE\n");

	while(running){
		printf("Running\n");
		sleep(2);
	}


	/* Kill the running procs generated for RARS and exit gracefully */
	//ignore the bin name but convert all the pids to int and kill with with SIGTERM
	for(int i=1; i<argc; i++){
		pid_t pid = atoi(argv[i]);
		printf("Killing pid: %d\n", pid);
		kill(pid, SIGTERM);
	}

	exit(EXIT_SUCCESS);


	name_attach_t* attach;
	struct _pulse msg;
	int rcvid;

	FILE *log_file;
	log_file = fopen("/tmp/display.log", "w");
	fprintf(log_file, "Starting display;\n");

	attach = name_attach(NULL, DISPLAY_SERVER, 0);
	if (attach == NULL){
		fprintf(log_file, "Could not start display\n");
		exit(EXIT_FAILURE);
	}

	while (running) {
		//receive message
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), &info);
		 if (0 == rcvid) {

			 switch(msg.code){
			 case _PULSE_CODE_DISCONNECT:
				fprintf(log_file, "Received disconnect from pulse\n");
				fflush(log_file);
				if (-1 == ConnectDetach(rbuf.pulse.scoid)) {
					perror("ConnectDetach");
				}
				break;
			 case TEMP_DATA:

			 default:
				 fprintf(log_file, "Unexpectes pulse code: %d", msg.code);
				 fflush(log_file);
			 }
//			//received a pulse
//			 if (msg.code == _PULSE_CODE_DISCONNECT){
//				printf("Process %d is gone\n", info.pid);
//				ConnectDetach(msg.pulse.scoid);
//				//break;
//			} else {
//				fprintf(log_file, "Message: %s\nFrom process: %d\n",msg.pulse.value.sival_ptr,info.pid);//might cause errors with sival_ptr
//				printf("Update: %s\n",msg.pulse.value.sival_ptr);//currently no compiler errors
//			}

		} else {
			fprintf(log_file, "Unexpected msg. Replying OK\n");
			fflush(log_file);
			MsgReply(rcvid, EOK, "OK", sizeof("OK"));
		}
	}

}

void cleanup_and_exit(int sig_no){
	running = 0;
}

