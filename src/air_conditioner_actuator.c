/*
 * air_conditioner_actuator.c
 *
 *	Driver for actuating the air conditioner.
 *	Receives messages from the the temperature_manager and sends commands to the environment simulation process
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
#include <time.h>


#include "constants.h"
#include "utilities.h"

/*
 * Struct for cmd messages
 */
typedef union
{
	uint16_t type;
	struct _pulse pulse;
	cmd_actu_chng_state_msg_t cmd;
} recv_cmd_t;

/* Track the running state in an atomic var modified by signal handler*/
volatile sig_atomic_t running = 1;


/* Functions */
void set_exit_flag(int sig_no);


int main(void) {
	FILE *log_file;
	int 			state = OFF;
	int 			rcvid;
	int				envSim_coid;
	name_attach_t 	*attach;
	recv_cmd_t 		rbuf;

	//set up log file
	log_file = fopen("/tmp/air_conditioner_actuator.log", "w");
	logString(log_file, "Starting air conditioner actuator");

	// register our name for a channel
	attach = name_attach(NULL, AIR_CONDITIONER_ACTUATOR_SERVER, 0);
	if (attach == NULL){
		logString(log_file, "Could not start server. %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	logString(log_file, "Starting Server");

	envSim_coid = name_open(ENVIRONMENT_SIMULATOR_SERVER, 0);
	if (envSim_coid == -1){
		logString(log_file, "Failed to connect to environment simulation server: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

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
			 default:
				 logString(log_file, "Unknown pulse received. Code: %d\n", rbuf.pulse.code);
			 }

		} else {
			// we got a message, check its type and process the msg based on its type
			switch(rbuf.type){
			case COMMAND_ACTUATOR_STATE:
				logString(log_file, "Received command: %d", rbuf.cmd.state);
				state = rbuf.cmd.state;

				// Send pulse to environment simulator
				if(MsgSendPulse(envSim_coid, -1, AIR_CONDITIONER_ACTUATOR_CHANGE, state) == 1){
					logString(log_file, "Failed to send state pulse to environment sim: %s", strerror(errno));
					exit(EXIT_FAILURE);
				}

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
	fclose(log_file);
	name_detach(attach, 0);
	return EXIT_SUCCESS;
}

void set_exit_flag(int sig_no){
	running = 0;
}
