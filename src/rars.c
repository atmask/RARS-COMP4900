#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/neutrino.h>
#include <unistd.h>
#include <sys/dispatch.h>
#include <string.h>

#include "constants.h"

#define NUM_PIDS 13

/********************************
 * Signals
 *******************************/
/*Atomic var to track running state*/
volatile sig_atomic_t running = 1;
void cleanup_and_exit(int);


int main(void) {
	/* Register signal handle to receive user INT*/
	signal(SIGINT, cleanup_and_exit);
	signal(SIGTERM, cleanup_and_exit);
	signal(SIGKILL, cleanup_and_exit);


	/*****************************************************************************
	 * Start the Display server so the manager can connect
	 *****************************************************************************/
	name_attach_t *attach;
	struct _pulse msg;
	int rcvid;

	printf("**************************************************************\n");
	printf("\tWELCOME TO THE REAL-TIME AGRICULTURAL SYSTEM (RARS)\n");
	printf("**************************************************************\n\n\n");


	attach = name_attach(NULL, DISPLAY_SERVER, 0);
	if (attach == NULL){
		printf("Could not start display\n");
		exit(EXIT_FAILURE);
	}
	/*****************************************************************************
	 * END: Display start
	 *****************************************************************************/


	pid_t rars_pids[NUM_PIDS];
	int temp_sensor_fd[2];// 0=RD 1=WR
	int humidity_sensor_fd[2];
	int ph_sensor_fd[2];

	/* Create the pipes representing the environment data*/
	if(pipe(temp_sensor_fd) == -1){
		perror("pipe(temp_sensor_fd) failed.");
		exit(EXIT_FAILURE);
	}
	if(pipe(humidity_sensor_fd) == -1){
		perror("pipe(humidity_sensor_fd) failed.");
		exit(EXIT_FAILURE);
	}
	if(pipe(ph_sensor_fd) == -1){
		perror("pipe(ph_sensor_fd) failed.");
		exit(EXIT_FAILURE);
	}

	/*****************************************************************************
	 * Create the environment simulator with the write ends of the pipes
	 *****************************************************************************/

	char temp_fd_str[10];
	char humidity_fd_str[10];
	char ph_fd_str[10];
	sprintf(temp_fd_str, "%d", temp_sensor_fd[1]);
	sprintf(humidity_fd_str, "%d", humidity_sensor_fd[1]);
	sprintf(ph_fd_str, "%d", ph_sensor_fd[1]);
	char *es_args[] = {"/tmp/environment_simulator", temp_fd_str, humidity_fd_str, ph_fd_str, NULL};
	rars_pids[0] = spawn("/tmp/environment_simulator", 0, NULL, NULL, es_args, NULL);
	if(rars_pids[0] == -1){
		perror("Failed to spawn environment simulator");
		exit(EXIT_FAILURE);
	}

	usleep(30);

	/*****************************************************************************
	 * Create the sensor drivers with the read ends of the pipes
	 *****************************************************************************/
	/*Create temp sensor proc*/
	char *ts_args[] = {"/tmp/temperature_sensor", NULL};
	int ts_fd_map[] = {temp_sensor_fd[0]};

	rars_pids[1] = spawn("/tmp/temperature_sensor", 1, ts_fd_map, NULL, ts_args, NULL);
	if(rars_pids[1] == -1){
		perror("Failed to spawn temp sensor");
		exit(EXIT_FAILURE);
	}

	/*Create humidity sensor proc*/
	char *hs_args[] = {"/tmp/humidity_sensor", NULL};
	int hs_fd_map[] = {humidity_sensor_fd[0]};

	rars_pids[2] = spawn("/tmp/humidity_sensor", 1, hs_fd_map, NULL, hs_args, NULL);
	if(rars_pids[2] == -1){
		perror("Failed to spawn humidity sensor");
		exit(EXIT_FAILURE);
	}

	/*Create pH sensor proc*/
	char *ps_args[] = {"/tmp/ph_sensor", NULL};
	int ph_fd_map[] = {ph_sensor_fd[0]};

	rars_pids[3] = spawn("/tmp/ph_sensor", 1, ph_fd_map, NULL, ps_args, NULL);
	if(rars_pids[3] == -1){
		perror("Failed to spawn pH sensor");
		exit(EXIT_FAILURE);
	}


	/*****************************************************************************
	 * Create the actuators
	 *****************************************************************************/
	//temp
	char *ac_args[] = {"/tmp/air_conditioner_actuator", NULL};
	rars_pids[4] = spawn("/tmp/air_conditioner_actuator", 0, NULL, NULL, ac_args, NULL);
	if(rars_pids[4] == -1){
		perror("Failed to spawn A/C actuator");
		exit(EXIT_FAILURE);
	}
	char *heater_args[] = {"/tmp/heater_actuator", NULL};
	rars_pids[5] = spawn("/tmp/heater_actuator", 0, NULL, NULL, heater_args, NULL);
	if(rars_pids[5] == -1){
		perror("Failed to spawn heater actuator");
		exit(EXIT_FAILURE);
	}

	//humid
	char *dehumidifier_args[] = {"/tmp/dehumidifier_actuator", NULL};
	rars_pids[6] = spawn("/tmp/dehumidifier_actuator", 0, NULL, NULL, dehumidifier_args, NULL);
	if(rars_pids[6] == -1){
		perror("Failed to spawn dehumidifier actuator");
		exit(EXIT_FAILURE);
	}
	char *humidifier_args[] = {"/tmp/humidifier_actuator", NULL};
	rars_pids[7] = spawn("/tmp/humidifier_actuator", 0, NULL, NULL, humidifier_args, NULL);
	if(rars_pids[7] == -1){
		perror("Failed to spawn humidifier actuator");
		exit(EXIT_FAILURE);
	}

	//pH
	char *fl_injector_args[] = {"/tmp/fl_injector_actuator", NULL};
	rars_pids[8] = spawn("/tmp/fl_injector_actuator", 0, NULL, NULL, fl_injector_args, NULL);
	if(rars_pids[8] == -1){
		perror("Failed to spawn flowable lime injector actuator");
		exit(EXIT_FAILURE);
	}
	char *as_injector_args[] = {"/tmp/as_injector_actuator", NULL};
	rars_pids[9] = spawn("/tmp/humidifier_actuator", 0, NULL, NULL, as_injector_args, NULL);
	if(rars_pids[9] == -1){
		perror("Failed to spawn aluminum sulfate injector actuator");
		exit(EXIT_FAILURE);
	}

	/*****************************************************************************
	 * Create the temp manager clients for the sensors
	 *****************************************************************************/
	//Temperature
	char *tm_args[] = {"/tmp/temperature_manager", TEMPERATURE_SENSOR_SERVER, NULL};
	rars_pids[10] = spawn("/tmp/temperature_manager", 0, NULL, NULL, tm_args, NULL);
	if(rars_pids[10] == -1){
		perror("Failed to spawn temp manager");
		exit(EXIT_FAILURE);
	}

	//Humidity
	char *hm_args[] = {"/tmp/humidity_manager", HUMIDITY_SENSOR_SERVER, NULL};
	rars_pids[11] = spawn("/tmp/humidity_manager", 0, NULL, NULL, hm_args, NULL);
	if(rars_pids[11] == -1){
		perror("Failed to spawn humidity manager");
		exit(EXIT_FAILURE);
	}

	//pH
	char *ph_args[] = {"/tmp/ph_manager", PH_SENSOR_SERVER, NULL};
	rars_pids[11] = spawn("/tmp/ph_manager", 0, NULL, NULL, ph_args, NULL);
	if(rars_pids[11] == -1){
		perror("Failed to spawn ph manager");
		exit(EXIT_FAILURE);
	}


	/*****************************************************************************
	 * Start the Display Logic
	 *****************************************************************************/

	//Store the data for the print
	//temp
	float temp;
	char ac_state[8];
	char heater_state[8];
	strcpy(ac_state, "OFF");
	strcpy(heater_state, "OFF");
	FILE *temp_metrics;
	temp_metrics = fopen("/tmp/temp_metrics.csv", "w");
	fprintf(temp_metrics, "temperature,max,min,a/c,heater\n");

	//humidity
	float humid;
	char humidifier[8];
	char dehumidifier[8];
	strcpy(humidifier, "OFF");
	strcpy(dehumidifier, "OFF");
	FILE *humid_metrics;
	humid_metrics = fopen("/tmp/humid_metrics.csv", "w");
	fprintf(humid_metrics, "humidity,max,min,humidifier,de-humidifier\n");

	//humidity
	float ph;
	char fl_injector[8];
	char as_injector[8];
	strcpy(fl_injector, "OFF");
	strcpy(as_injector, "OFF");
	FILE *ph_metrics;
	ph_metrics = fopen("/tmp/ph_metrics.csv", "w");
	fprintf(ph_metrics, "ph,max,min,fl-injector,as-injector\n");

	// Create a timer to periodically print the display
	struct sigevent sigevent;
	struct itimerspec itime;
	timer_t timerID;
	int event_coid;
	event_coid = ConnectAttach(0, 0, attach->chid, _NTO_SIDE_CHANNEL, 0);
	SIGEV_PULSE_INIT(&sigevent, event_coid, SIGEV_PULSE_PRIO_INHERIT, TIMER_PULSE_CODE, 0);
	timer_create(CLOCK_REALTIME, &sigevent, &timerID);
	itime.it_value.tv_sec = 2;
	itime.it_value.tv_nsec = 0;
	itime.it_interval.tv_sec = 2;
	itime.it_interval.tv_nsec = 0;
	timer_settime(timerID, 0, &itime, NULL);

	while (running) {
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), 0);
		 if (0 == rcvid) {
			 switch(msg.code){
			 case _PULSE_CODE_DISCONNECT:
				printf("Received disconnect from pulse\n");
				if (-1 == ConnectDetach(msg.scoid)) {
					perror("ConnectDetach");
				}
				break;
			 case TIMER_PULSE_CODE:
				 printf("\n\n\n\n\n\n\n\n\n");
				 printf("************************************************************\n\t\tSENSORS\n************************************************************\n");
				 printf("Temperature Sensor: %.2f\tMax: %d\tMin: %d\n\n", temp, MAX_TEMP, MIN_TEMP);
				 printf("Humidity Sensor: %.2f\t\tMax: %d\tMin: %d\n\n", humid, MAX_HUMID, MIN_HUMID);
				 printf("pH Sensor: %.2f\t\t\tMax: %d\tMin: %d\n\n", ph, MAX_PH, MIN_PH);
				 printf("************************************************************\n\t\tACTUATORS\n************************************************************\n");
				 printf("A/C Unit:\t\t\t%s\n", ac_state);
				 printf("Heating Unit:\t\t\t%s\n\n", heater_state);
				 printf("Dehumidifier Unit:\t\t%s\n", dehumidifier);
 				 printf("Humidifier Unit:\t\t%s\n\n", humidifier);
 				 printf("Flowable Lime Injector Unit:\t%s\n", fl_injector);
 				 printf("Aluminum Sulfate Injector Unit:\t%s\n\n", as_injector);

				 break;
			 case KILL_ALL:
			 	 printf("Pulse received:\n[DISPLAY] Killing all processes\n");
			 	 break;
			 case TEMP_DATA:
				 //printf("Pulse received:\n[DISPLAY] Temperature Sensor: %d\n", msg.value);
				 temp = msg.value.sival_int;
				 fprintf(temp_metrics, "%.2f,%d,%d,%s,%s\n", temp, MAX_TEMP, MIN_TEMP, ac_state, heater_state);
				 break;
			 case TEMP_AC:
				 if(msg.value.sival_int == ON){
					strcpy(ac_state, "ON");
					//printf("Pulse received:\n[DISPLAY] AC has turned on\n");
				 }else if(msg.value.sival_int == OFF){
					strcpy(ac_state, "OFF");
					//printf("Pulse received:\n[DISPLAY] AC has turned off\n");
				 }
				 fprintf(temp_metrics, "%.2f,%d,%d,%s,%s\n", temp, MAX_TEMP, MIN_TEMP, ac_state, heater_state);

				 break;
			 case TEMP_HEATER:
				 //heater_state = sg.value.sival_int;
				 if(msg.value.sival_int == ON){
					strcpy(heater_state, "ON");
					//printf("Pulse received:\n[DISPLAY] Heater has turned on\n");
				 }else if(msg.value.sival_int == OFF){
					strcpy(heater_state, "OFF");
					// printf("Pulse received:\n[DISPLAY] Heater has turned off\n");
				 }
				 fprintf(temp_metrics, "%.2f,%d,%d,%s,%s\n", temp, MAX_TEMP, MIN_TEMP, ac_state, heater_state);
			 	 break;
			 case HUMID_DATA:
			 	 //printf("Pulse received:\n[DISPLAY] Temperature Sensor: %d\n", msg.value);
			 	 humid = msg.value.sival_int;
				 fprintf(humid_metrics, "%.2f,%d,%d,%s,%s\n", humid, MAX_HUMID, MIN_HUMID, humidifier, dehumidifier);
			 	 break;
			 case HUMID_DEHUMIDIFIER:
			 	 if(msg.value.sival_int == ON){
			 		strcpy(dehumidifier, "ON");
			 		//printf("Pulse received:\n[DISPLAY] AC has turned on\n");
			 	 }else if(msg.value.sival_int == OFF){
			 		strcpy(dehumidifier, "OFF");
			 		//printf("Pulse received:\n[DISPLAY] AC has turned off\n");
			 	 }
			 	fprintf(humid_metrics, "%.2f,%d,%d,%s,%s\n", humid, MAX_HUMID, MIN_HUMID, humidifier, dehumidifier);
			  	break;
			  case HUMID_HUMIDIFIER:
			 	 //heater_state = sg.value.sival_int;
			 	 if(msg.value.sival_int == ON){
			 		strcpy(humidifier, "ON");
			 		//printf("Pulse received:\n[DISPLAY] Heater has turned on\n");
			 	 }else if(msg.value.sival_int == OFF){
			 		strcpy(humidifier, "OFF");
			 		// printf("Pulse received:\n[DISPLAY] Heater has turned off\n");
			 	 }
			 	 fprintf(humid_metrics, "%.2f,%d,%d,%s,%s\n", humid, MAX_HUMID, MIN_HUMID, humidifier, dehumidifier);
			  	 break;
			 case PH_DATA:
				 //printf("Pulse received:\n[DISPLAY] Temperature Sensor: %d\n", msg.value);
				 ph = msg.value.sival_int;
				 fprintf(ph_metrics, "%.2f,%d,%d,%s,%s\n", ph, MAX_PH, MIN_PH, fl_injector, as_injector);
				 break;
			 case PH_AS_INJECTOR:
				 if(msg.value.sival_int == ON){
					strcpy(as_injector, "ON");
					//printf("Pulse received:\n[DISPLAY] AC has turned on\n");
				 }else if(msg.value.sival_int == OFF){
					strcpy(as_injector, "OFF");
					//printf("Pulse received:\n[DISPLAY] AC has turned off\n");
				 }
				 fprintf(ph_metrics, "%.2f,%d,%d,%s,%s\n", ph, MAX_PH, MIN_PH, fl_injector, as_injector);
				break;
			  case PH_FL_INJECTOR:
				 //heater_state = sg.value.sival_int;
				 if(msg.value.sival_int == ON){
					strcpy(fl_injector, "ON");
					//printf("Pulse received:\n[DISPLAY] Heater has turned on\n");
				 }else if(msg.value.sival_int == OFF){
					strcpy(fl_injector, "OFF");
					// printf("Pulse received:\n[DISPLAY] Heater has turned off\n");
				 }
				 fprintf(ph_metrics, "%.2f,%d,%d,%s,%s\n", ph, MAX_PH, MIN_PH, fl_injector, as_injector);
				 break;
			 default:
				 printf("Unexpected pulse code: %d", msg.code);
				 break;
			 }
		} else {
			printf("Unexpected msg. Replying OK\n");
			MsgReply(rcvid, EOK, "OK", sizeof("OK"));
		}
	}

	/*Clean up*/
	fclose(temp_metrics);
	fclose(humid_metrics);
	fclose(ph_metrics);
	name_detach(attach, 0);

	/* Kill the running procs generated for RARS and exit gracefully */
	//ignore the bin name but convert all the pids to int and kill with with SIGTERM
	for(int i=0; i<NUM_PIDS; i++){
		printf("Killing pid: %d\n", rars_pids[i]);
		kill(rars_pids[i], SIGTERM);
	}


	return EXIT_SUCCESS;
}


void cleanup_and_exit(int sig_no){
	printf("Interrupt: Exiting\n");
	running = 0;
}

