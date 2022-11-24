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

int main(int argc, char **argv){
	FILE *log_file;
	log_file = fopen("/tmp/temp_manager.log", "w");
	logString(log_file, "Starting temp manager");

	int coid;

	/* Connect to the temperature sensor server */
	coid = name_open(TEMPERATURE_SENSOR_SERVER, 0);
	if (coid == -1){
		logString(log_file, "Failed to connect to server. Code: %s\n", strerror(errno));
		fclose(log_file);
		exit(EXIT_FAILURE);
	}

	/* Build the GET message */
	get_snsr_data__msg_t get_msg;
	get_msg.type = GET_DATA;
	resp_snsr_data_msg_t resp;
	MsgSend(coid, &get_msg, sizeof(get_msg), &resp, sizeof(resp));
	logString(log_file, "GOT DATA: %.2f", resp.data);

	name_close(coid);
	return EXIT_SUCCESS;
}
