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

//using printf for outputs for now
int main(int argc, char **argv){
	typedef union
	{
		struct _pulse pulse;
		uint16_t type;//could be useful
	} myMsg_t;
	myMsg_t msg;
	int rcvid;

	FILE *log_file;
	struct _msg_info info;
	log_file = fopen("/tmp/display.log", "w");
	logString(log_file, "Starting display;\n");

	printf("RARS starting\nAttempting to Start Systems\n\n");

	name_attach_t* attach;
	attach = name_attach(NULL, DISPLAY, 0);
	if (attach == NULL){
		logString(log_file, "Could not start display\n");
		printf("Display error. Closing display\n");
		exit(EXIT_FAILURE);
	}

	while (1) {
		//receive message
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), &info);
		 if (0 == rcvid) {
			//received a pulse
			 if (msg.pulse.code == _PULSE_CODE_DISCONNECT){
				printf("Process %d is gone\n", info.pid);
				ConnectDetach(msg.pulse.scoid);
				//break;
			} else {
				logString(log_file, "Message: %s\nFrom process: %d\n",msg.pulse.value.sival_ptr,info.pid);//might cause errors with sival_ptr
				printf("Update: %s\n",msg.pulse.value.sival_ptr);//currently no compiler errors
			}

		}
	}

}

