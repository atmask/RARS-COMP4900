/*
 * humidity_manager.c
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
 * Humidity Manager
 *
 * Take the sensor server name as first arg and actuator as second
 */
int main(int argc, char **argv){
	FILE *log_file;
	log_file = fopen("/tmp/humid_manager.log", "w");
	logString(log_file, "Starting humid manager");
	int humid_coid, display_coid, dehumidifier_coid, humidifier_coid;
	int dehumidifier_state = OFF;
	int humidifier_state = OFF;

	/* Connect to the humidity sensor server */
	humid_coid = name_open(HUMIDITY_SENSOR_SERVER, 0);
	if (humid_coid == -1){
		logString(log_file, "Failed to connect to humid server: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}


	/* Connect to the display server */
	display_coid = name_open(DISPLAY_SERVER, 0);
	if (display_coid == -1){
		logString(log_file, "Failed to connect to display server: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Connect to the dehumidifier server */
	humidifier_coid = name_open(HUMIDIFIER_ACTUATOR_SERVER, 0);
	if (humidifier_coid == -1){
		logString(log_file, "Failed to connect to humidifier server: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Connect to the humidifier server */
	dehumidifier_coid = name_open(DEHUMIDIFIER_ACTUATOR_SERVER, 0);
	if (dehumidifier_coid == -1){
		logString(log_file, "Failed to connect to dehumidifier server: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}


	while(running){
		/*****************************************
		 * GET SENSOR DATA
		 *****************************************/
		get_snsr_data__msg_t get_msg;
		get_msg.type = GET_DATA;
		resp_snsr_data_msg_t resp;
		if (MsgSend(humid_coid, &get_msg, sizeof(get_msg), &resp, sizeof(resp)) == -1){
			logString(log_file, "Failed to get data from sensor: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		logString(log_file, "GOT DATA: %.2f", resp.data);


		/*****************************************
		 * PULSE TO DISLPAY
		 *****************************************/
		if(MsgSendPulse(display_coid, -1, HUMID_DATA, resp.data) == 1){
			logString(log_file, "Failed to connect to send data pulse to display: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		logString(log_file, "Pulsed DATA: %.2f", resp.data);

		/*****************************************
		 * CHECK THRESHOLDS AND ACTUATORS
		 *****************************************/
		if(resp.data > MAX_HUMID*100){
			logString(log_file, "Above max humid");


			// Turn the dehumidifier Unit ON
			if(dehumidifier_state == OFF){
				logString(log_file, "TURN DEHUMIDIFIER ON");

				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = ON;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(dehumidifier_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send ON to dehumidifier: %s", strerror(errno));
					exit(EXIT_FAILURE);
				}
				dehumidifier_state = resp_state.state;
			}

			// Turn the humidifier Unit OFF
			if(humidifier_state == ON){
				logString(log_file, "TURN HUMIDIFIER OFF");


				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = OFF;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(humidifier_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send OFF to humidifier: %s", strerror(errno));
					exit(EXIT_FAILURE);
				}
				humidifier_state = resp_state.state;

			}



		} else if (resp.data < MIN_HUMID*100){
			logString(log_file, "Below min humid");


			// Turn the dehumidifier Unit OFF
			if(dehumidifier_state == ON){
				logString(log_file, "TURN DEHUMIDIFIER OFF");


				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = OFF;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(dehumidifier_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send OFF to dehumidifier: %s", strerror(errno));
					exit(EXIT_FAILURE);
				}
				dehumidifier_state = resp_state.state;
			}

			// Turn the humidifier Unit ON
			if(humidifier_state == OFF){
				logString(log_file, "TURN HUMIDIFIER ON");


				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = ON;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(humidifier_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send ON to humidifier: %s", strerror(errno));
					exit(EXIT_FAILURE);
				}
				humidifier_state = resp_state.state;

			}


		} else {

			// Turn the dehumidifier Unit OFF
			if(dehumidifier_state == ON){
				logString(log_file, "TURN DEHUMIDIFIER OFF");


				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = OFF;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(dehumidifier_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send OFF to dehumidifier: %s", strerror(errno));
					exit(EXIT_FAILURE);
				}
				dehumidifier_state = resp_state.state;
			}

			// Turn the humidifier Unit OFF
			if(humidifier_state == ON){
				logString(log_file, "TURN HUMIDIFIER OFF");

				/*Build actuator msg*/
				cmd_actu_chng_state_msg_t msg;
				msg.type = COMMAND_ACTUATOR_STATE;
				msg.state = OFF;

				resp_actu_state_msg_t resp_state;
				if(MsgSend(humidifier_coid, &msg, sizeof(msg), &resp_state, sizeof(resp_state)) == -1){
					logString(log_file, "Failed to connect to send OFF to humidifier: %s", strerror(errno));
					exit(EXIT_FAILURE);
				}
				humidifier_state = resp_state.state;

			}
		}

		/***************************************
		 * PULSE THE UPDATED ACTUATOR STATES
		 ****************************************/
		if(MsgSendPulse(display_coid, -1, HUMID_DEHUMIDIFIER, dehumidifier_state) == 1){
			logString(log_file, "Failed to send dehumidifier actuator state pulse to display: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		if(MsgSendPulse(display_coid, -1, HUMID_HUMIDIFIER, humidifier_state) == 1){
			logString(log_file, "Failed to send humidifier actuator state pulse to display: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		sleep(2);

	}

	/* Send terminate pulse to the server and actuator and cleanup */

	fclose(log_file);
	name_close(humid_coid);
	name_close(display_coid);
	return EXIT_SUCCESS;
}

void set_exit_flag(int sig_no){
	running = 0;
}
