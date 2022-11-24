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

	printf("HERE\n");
	/* Register signal handle to receive user INT*/
	signal(SIGINT, cleanup_and_exit);
	signal(SIGTERM, cleanup_and_exit);
	signal(SIGKILL, cleanup_and_exit);

	name_attach_t* attach;
	struct _pulse msg;
	int rcvid;

	printf("Starting display;\n");

	attach = name_attach(NULL, DISPLAY_SERVER, 0);
	if (attach == NULL){
		printf("Could not start display\n");
		exit(EXIT_FAILURE);
	}

	while (running) {
		//receive message
		printf("waiting for data pulse\n");
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), 0);
		 if (0 == rcvid) {

			 switch(msg.code){
			 case _PULSE_CODE_DISCONNECT:
				printf("Received disconnect from pulse\n");
				if (-1 == ConnectDetach(msg.scoid)) {
					perror("ConnectDetach");
				}
				break;
			 case TEMP_DATA:
				 printf("GOT TEMP DATA: %d", msg.value);
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
	for(int i=1; i<argc; i++){
		pid_t pid = atoi(argv[i]);
		printf("Killing pid: %d\n", pid);
		kill(pid, SIGTERM);
	}

	exit(EXIT_SUCCESS);


}

void cleanup_and_exit(int sig_no){
	running = 0;
}

