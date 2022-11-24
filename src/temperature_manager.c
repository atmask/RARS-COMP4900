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
	logString(log_file, "Starting temp manager");
	int coid;

	/* Connect to the temperature sensor server */
	coid = name_open(TEMPERATURE_SENSOR_SERVER, 0);
	if (coid == -1){
		logString(log_file, "Failed to connect to server. Code: %s", strerror(errno));
		fclose(log_file);
		exit(EXIT_FAILURE);
	}

	while(running){
		/* Build the GET message */
		get_snsr_data__msg_t get_msg;
		get_msg.type = GET_DATA;
		resp_snsr_data_msg_t resp;
		MsgSend(coid, &get_msg, sizeof(get_msg), &resp, sizeof(resp));
		logString(log_file, "GOT DATA: %.2f", resp.data);
		sleep(2);

	}

	/* Send terminate pulse to the server and actuator and cleanup */

	fclose(log_file);
	name_close(coid);
	return EXIT_SUCCESS;
}


void set_exit_flag(int sig_no){
	running = 0;
}
