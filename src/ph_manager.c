/*
 * ph_manager.c
 *
 *  Created on: Dec 2, 2022
 *      Author: everyone
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


/* Track the running state in an atomic var modified by signal handler*/
volatile sig_atomic_t running = 1;


/* Functions */
void set_exit_flag(int sig_no);

/**
 * ph Manager
 *
 * Take the sensor server name as first arg and actuator as second
 */
int main(int argc, char **argv){
	FILE *log_file;
	log_file = fopen("/tmp/ph_manager.log", "w");
	logString(log_file, "Starting ph manager");
	int ph_coid, display_coid, as_injector_coid, fl_injector_coid;
	int as_injector_state = OFF;
	int fl_injector_state = OFF;

	/* Connect to the ph sensor server */
	ph_coid = name_open(PH_SENSOR_SERVER, 0);
	if (ph_coid == -1){
		logString(log_file, "Failed to connect to ph server: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}


	/* Connect to the display server */
	display_coid = name_open(DISPLAY_SERVER, 0);
	if (display_coid == -1){
		logString(log_file, "Failed to connect to display server: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Connect to the as_injector server */
	fl_injector_coid = name_open(FL_INJECTOR_ACTUATOR_CHANGE, 0);
	if (fl_injector_coid == -1){
		logString(log_file, "Failed to connect to fl_injector server: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Connect to the fl_injector server */
	as_injector_coid = name_open(AS_INJECTOR_ACTUATOR_CHANGE, 0);
	if (as_injector_coid == -1){
		logString(log_file, "Failed to connect to as_injector server: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}


	while(running){
		/*****************************************
		 * GET SENSOR DATA
		 *****************************************/
		get_snsr_data__msg_t get_msg;
		get_msg.type = GET_DATA;
		resp_snsr_data_msg_t resp;
		if (MsgSend(ph_coid, &get_msg, sizeof(get_msg), &resp, sizeof(resp)) == -1){
			logString(log_file, "Failed to get data from sensor: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		logString(log_file, "GOT DATA: %.2f", resp.data);


		/*****************************************
		 * PULSE TO DISLPAY
		 *****************************************/
		if(MsgSendPulse(display_coid, -1, PH_DATA, resp.data) == 1){
			logString(log_file, "Failed to connect to send data pulse to display: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		logString(log_file, "Pulsed DATA: %.2f", resp.data);

		/*****************************************
		 * CHECK THRESHOLDS AND ACTUATORS
		 *****************************************/
		if(resp.data > MAX_PH){
			logString(log_file, "Above max ph");


			// Turn the as_injector Unit ON
			if(as_injector_state == OFF){
				logString(log_file, "TURN AS INJECTOR ON");

				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = ON;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(as_injector_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send ON to as_injector: %s", strerror(errno));
					exit(EXIT_FAILURE);
				}
				as_injector_state = resp_state.state;
			}

			// Turn the fl_injector Unit OFF
			if(fl_injector_state == ON){
				logString(log_file, "TURN FL INJECTOR OFF");


				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = OFF;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(fl_injector_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send OFF to fl_injector: %s", strerror(errno));
					exit(EXIT_FAILURE);
				}
				fl_injector_state = resp_state.state;

			}



		} else if (resp.data < MIN_PH){
			logString(log_file, "Below min ph");


			// Turn the as_injector Unit OFF
			if(as_injector_state == ON){
				logString(log_file, "TURN AS INJECTOR OFF");


				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = OFF;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(as_injector_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send OFF to as_injector: %s", strerror(errno));
					exit(EXIT_FAILURE);
				}
				as_injector_state = resp_state.state;
			}

			// Turn the fl_injector Unit ON
			if(fl_injector_state == OFF){
				logString(log_file, "TURN FL INJECTOR ON");


				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = ON;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(fl_injector_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send ON to fl_injector: %s", strerror(errno));
					exit(EXIT_FAILURE);
				}
				fl_injector_state = resp_state.state;

			}


		} else {

			// Turn the as_injector Unit OFF
			if(as_injector_state == ON){
				logString(log_file, "TURN AS INJECTOR OFF");


				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = OFF;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(as_injector_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send OFF to as_injector: %s", strerror(errno));
					exit(EXIT_FAILURE);
				}
				as_injector_state = resp_state.state;
			}

			// Turn the fl_injector Unit OFF
			if(fl_injector_state == ON){
				logString(log_file, "TURN FL INJECTOR OFF");

				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = OFF;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(fl_injector_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send OFF to fl_injector: %s", strerror(errno));
					exit(EXIT_FAILURE);
				}
				fl_injector_state = resp_state.state;

			}
		}

		/***************************************
		 * PULSE THE UPDATED ACTUATOR STATES
		 ****************************************/
		if(MsgSendPulse(display_coid, -1, PH_AS_INJECTOR, as_injector_state) == 1){
			logString(log_file, "Failed to send as_injector actuator state pulse to display: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		if(MsgSendPulse(display_coid, -1, PH_FL_INJECTOR, fl_injector_state) == 1){
			logString(log_file, "Failed to send fl_injector actuator state pulse to display: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		sleep(2);

	}

	/* Send terminate pulse to the server and actuator and cleanup */

	fclose(log_file);
	name_close(ph_coid);
	name_close(display_coid);
	return EXIT_SUCCESS;
}

void set_exit_flag(int sig_no){
	running = 0;
}
