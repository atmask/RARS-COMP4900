/*
 * heater_actuator.c
 *
 *	Driver for actuating the heater.
 *	Receives messages from the the temperature_actuator meta-driver and sends commands to the environment simulation process
 *
 *  Created on: Nov. 22, 2022
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
 * Struct for get messages
 */
typedef union
{
	uint16_t type;
	struct _pulse pulse;
	cmd_actu_chng_state_msg_t cmd;
} recv_cmd_t;

int main(void) {
	FILE *log_file;
	int 			state = OFF;
	int 			rcvid;
	name_attach_t 	*attach;
	recv_cmd_t 		rbuf;

	//set up log file
	log_file = fopen("/tmp/heater_actuator.log", "w");
	logString(log_file, "Starting heater actuator");

	// register our name for a channel
	attach = name_attach(NULL, HEATER_ACTUATOR_SERVER, 0);
	if (attach == NULL){
		logString(log_file, "Could not start server");
		exit(EXIT_FAILURE);
	}
	logString(log_file, "Starting Server");


	while (1) {
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
			 default:
				 logString(log_file, "Unknown pulse received. Code: %d\n", rbuf.pulse.code);
			 }

		} else {
			long status = EOK;
			// we got a message, check its type and process the msg based on its type
			switch(rbuf.type){
			case COMMAND_ACTUATOR_STATE:
				logString(log_file, "Received command: %d", rbuf.cmd.state);
				if(rbuf.cmd.state == UP || rbuf.cmd.state == OFF)
				{
					// Change the state of the actuator
					state = rbuf.cmd.state;

					//TODO: SEND MESSAGE TO ENVIRONMENT SIMULATOR
				}
				else
				{
					logString(log_file, "ERROR: %d is an invalid state for heater", rbuf.cmd.state);
					status = EPERM;
				}

				/* Build the response */
				resp_actu_state_msg_t resp;
				resp.state = state;
				logString(log_file, "Replying with current state: %d", state);

				/* Send data back */
				MsgReply(rcvid, status, &resp, sizeof(resp));
				break;
			default:
				logString(log_file, "Received unknown message type: %d", rbuf.type);
				MsgReply(rcvid, ENOTSUP, "Not supported", sizeof("Not supported"));
			}
		}
	}
	//TODO: Clean up with name_dettatch in the sigin handler
	return 0;
}
