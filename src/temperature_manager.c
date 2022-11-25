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
	int temp_coid, display_coid;

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


	while(running){
		/* Build the GET message */
		get_snsr_data__msg_t get_msg;
		get_msg.type = GET_DATA;
		resp_snsr_data_msg_t resp;
		MsgSend(temp_coid, &get_msg, sizeof(get_msg), &resp, sizeof(resp));
		logString(log_file, "GOT DATA: %.2f\n", resp.data);


		/* Build Pulse for the display manager */
		if(MsgSendPulse(display_coid, -1, TEMP_DATA, resp.data) == 1){
			logString(log_file, "Failed to connect to send data pulse to display: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		logString(log_file, "Pulsed DATA: %.2f. Now sleeping 2 seconds\n", resp.data);

		/* Check the data against thresholds*/
		if(resp.data > MAX_TEMP){
			logString(log_file, "Above max temp");
		} else if (resp.data < MIN_TEMP){
			logString(log_file, "Below min temp");
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
