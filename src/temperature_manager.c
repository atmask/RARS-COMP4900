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
 * Temperature Manager
 *
 * Take the sensor server name as first arg and actuator as second
 */
int main(int argc, char **argv){
	FILE *log_file;
	log_file = fopen("/tmp/temp_manager.log", "w");
	logString(log_file, "Starting temp manager\n");
	int temp_coid, display_coid, ac_coid, heater_coid;
	int ac_state = OFF;
	int heater_state = OFF;

	/* Connect to the temperature sensor server */
	temp_coid = name_open(TEMPERATURE_SERVER, 0);
	if (temp_coid == -1){
		logString(log_file, "Failed to connect to temp server: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}


	/* Connect to the display server */
	display_coid = name_open(DISPLAY_SERVER, 0);
	if (display_coid == -1){
		logString(log_file, "Failed to connect to display server: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Connect to the A/C server */
	ac_coid = name_open(AIR_CONDITIONER_ACTUATOR_SERVER, 0);
	if (ac_coid == -1){
		logString(log_file, "Failed to connect to A/C server: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Connect to the heater server */
	heater_coid = name_open(HEATER_ACTUATOR_SERVER, 0);
	if (ac_coid == -1){
		logString(log_file, "Failed to connect to heater server: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}


	while(running){
		/*****************************************
		 * GET SENSOR DATA
		 *****************************************/
		get_snsr_data__msg_t get_msg;
		get_msg.type = GET_DATA;
		resp_snsr_data_msg_t resp;
		if (MsgSend(temp_coid, &get_msg, sizeof(get_msg), &resp, sizeof(resp)) == -1){
			logString(log_file, "Failed to get data from sensor: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		logString(log_file, "GOT DATA: %.2f\n", resp.data);


		/*****************************************
		 * PULSE TO DISLPAY
		 *****************************************/
		if(MsgSendPulse(display_coid, -1, TEMP_DATA, resp.data) == 1){
			logString(log_file, "Failed to connect to send data pulse to display: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		logString(log_file, "Pulsed DATA: %.2f\n", resp.data);

		/*****************************************
		 * CHECK THRESHOLDS AND ACTUATORS
		 *****************************************/
		if(resp.data > MAX_TEMP){
			logString(log_file, "Above max temp\n");


			// Turn the A/C Unit ON
			if(ac_state == OFF){
				logString(log_file, "TURN AC ON\n");

				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = ON;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(ac_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send ON to A/C: %s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				ac_state = resp_state.state;
			}

			// Turn the heater Unit OFF
			if(heater_state == ON){
				logString(log_file, "TURN HEATER OFF\n");


				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = OFF;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(heater_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send OFF to Heater: %s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				heater_state = resp_state.state;

			}



		} else if (resp.data < MIN_TEMP){
			logString(log_file, "Below min temp\n");


			// Turn the A/C Unit ON
			if(ac_state == ON){
				logString(log_file, "TURN AC OFF\n");


				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = OFF;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(ac_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send OFF to A/C: %s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				ac_state = resp_state.state;
			}

			// Turn the heater Unit OFF
			if(heater_state == OFF){
				logString(log_file, "TURN HEATER ON\n");


				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = ON;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(heater_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send ON to Heater: %s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				heater_state = resp_state.state;

			}


		} else {

			// Turn the A/C Unit ON
			if(ac_state == ON){
				logString(log_file, "TURN AC OFF\n");


				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = OFF;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(ac_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send OFF to A/C: %s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				ac_state = resp_state.state;
			}

			// Turn the heater Unit OFF
			if(heater_state == ON){
				logString(log_file, "TURN HEATER OFF\n");

				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = OFF;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(heater_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send OFF to Heater: %s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				heater_state = resp_state.state;

			}
		}

		/***************************************
		 * PULSE THE UPDATED ACTUATOR STATES
		 ****************************************/
		if(MsgSendPulse(display_coid, -1, TEMP_AC, ac_state) == 1){
			logString(log_file, "Failed to send AC actuator state pulse to display: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		if(MsgSendPulse(display_coid, -1, TEMP_HEATER, heater_state) == 1){
			logString(log_file, "Failed to send HEATER actuator state pulse to display: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		sleep(2);

	}

	/* Send terminate pulse to the server and actuator and cleanup */

	fclose(log_file);
	name_close(temp_coid);
	name_close(display_coid);
	return EXIT_SUCCESS;
}

void set_exit_flag(int sig_no){
	running = 0;
}
