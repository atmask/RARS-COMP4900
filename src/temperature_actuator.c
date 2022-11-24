/*
 * temperature_actuator.c
 *
 *	Meta-driver wrapping the heater and cooler actuators.
 *	Receives messages from the temperature_manager and sends commands to the two actuators.
 *
 *  Created on: Nov. 21, 2022
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
#include <time.h>


#include "constants.h"
#include "utilities.h"

/*
 * Struct for state change commands
 */
typedef union
{
	uint16_t type;
	struct _pulse pulse;
	cmd_actu_chng_state_msg_t cmd;
} recv_cmd_t;


FILE *log_file;

/*
 * Send messages to A/C and Heater according to desired state
 */
int delegateStateChange(int state)
{
	int 	coid;

	/* Connect to the air conditioner server */
	coid = name_open(AIR_CONDITIONER_ACTUATOR_SERVER, 0);
	if (coid == -1){
		logString(log_file, "Failed to connect to server. Code: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	cmd_actu_chng_state_msg_t cmd;
	cmd.state = state;
	cmd.type = COMMAND_ACTUATOR_STATE;
	resp_actu_state_msg_t ac_resp;
	MsgSend(coid, &cmd, sizeof(cmd), &ac_resp, sizeof(ac_resp));
	name_close(coid);

	/* Connect to the heater server */
	coid = name_open(HEATER_ACTUATOR_SERVER, 0);
	if (coid == -1){
		logString(log_file, "Failed to connect to server. Code: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	resp_actu_state_msg_t heater_resp;
	MsgSend(coid, &cmd, sizeof(cmd), &heater_resp, sizeof(heater_resp));
	name_close(coid);

	if(ac_resp.state == state && heater_resp.state == state)
	{
		return EXIT_SUCCESS;
	}
	else
	{
		return EXIT_FAILURE;
	}
}

int main(void) {

	int 			rcvid;
	name_attach_t 	*attach;
	recv_cmd_t 		rbuf;
	int 			state = OFF;

	//set up log file
	log_file = fopen("/tmp/temperature_actuator.log", "w");
	logString(log_file, "Starting temp actuator");

	// register our name for a channel
	attach = name_attach(NULL, TEMPERATURE_ACTUATOR_SERVER, 0);
	if (attach == NULL){
		logString(log_file, "Could not start server. %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	logString(log_file, "Starting Server");
	return EXIT_SUCCESS;
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
			// we got a message, check its type and process the msg based on its type
			switch(rbuf.type){
			case COMMAND_ACTUATOR_STATE:
				// Change the state of the actuator
				state = rbuf.cmd.state;
				logString(log_file, "Received command: %d", state);

				//TODO: SEND MESSAGES TO HEATER AND A/C

				/* Build the response */
				resp_actu_state_msg_t resp;
				resp.state = state;
				logString(log_file, "Replying with current state: %d", state);

				/* Send data back */
				MsgReply(rcvid, EOK, &resp, sizeof(resp));
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




