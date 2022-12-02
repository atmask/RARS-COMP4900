/*
 * constansts.h
 *
 *  Created on: Nov. 18, 2022
 */

#ifndef SRC_CONSTANTS_H_
#define SRC_CONSTANTS_H_

#include <stdint.h>
#include <sys/iomsg.h>
#include <sys/mman.h>

/**********************************************
 * SERVER NAMES
 ********************************************/
#define DISPLAY_SERVER "display"
#define TEMPERATURE_SENSOR_SERVER "temperature_sensor"
#define HEATER_ACTUATOR_SERVER "heater_actuator"
#define AIR_CONDITIONER_ACTUATOR_SERVER "air_conditioner_actuator"
#define HUMIDIFIER_ACTUATOR_SERVER "humidifier_actuator"
#define DEHUMIDIFIER_ACTUATOR_SERVER "dehumidifier_actuator"
#define ENVIRONMENT_SIMULATOR_SERVER "environment_simulator"
#define HUMIDITY_SENSOR_SERVER "humidity_sensor"

/**********************************************
 * MESSAGE TYPES
 ********************************************/
#define GET_DATA (_IO_MAX+1)
#define SENSOR_DATA (_IO_MAX+2)
#define COMMAND_ACTUATOR_STATE (_IO_MAX+3)

/**********************************************
 * SENSOR SERVER MESSAGE STRUCTS
 ********************************************/
// Message structs to ask for temp data
typedef struct get_snsr_data_msg {
	uint16_t type;
} get_snsr_data__msg_t;

typedef struct resp_snsr_data_msg {
	float		data;
} resp_snsr_data_msg_t;


// Message structs to command actuator to change states
typedef struct cmd_actu_chng_state_msg {
	uint16_t type;
	int		state;	// UP, DOWN, OFF
} cmd_actu_chng_state_msg_t;

typedef struct resp_actu_state_msg {
	int		state;	// UP, DOWN, OFF
} resp_actu_state_msg_t;


/*********************************
 *  PULSE CODES FOR DISPLAY
 *********************************/
#define KILL_ALL (_PULSE_CODE_MINAVAIL+1)
//temp vars
#define TEMP_DATA (_PULSE_CODE_MINAVAIL+2)
#define TEMP_AC (_PULSE_CODE_MINAVAIL+3)
#define TEMP_HEATER (_PULSE_CODE_MINAVAIL+4)
//timer
#define TIMER_PULSE_CODE (_PULSE_CODE_MINAVAIL+5)
//humid vars
#define HUMID_DATA (_PULSE_CODE_MINAVAIL+6)
#define HUMID_HUMIDIFIER (_PULSE_CODE_MINAVAIL+7)
#define HUMID_DEHUMIDIFIER (_PULSE_CODE_MINAVAIL+8)

/*********************************************
 *    PULSE CODES FOR ENVIRONMENT SIMULATOR
 *********************************************/
#define AIR_CONDITIONER_ACTUATOR_CHANGE (_PULSE_CODE_MINAVAIL+9)
#define HEATER_ACTUATOR_CHANGE (_PULSE_CODE_MINAVAIL+10)
#define HUMIDIFIER_ACTUATOR_CHANGE (_PULSE_CODE_MINAVAIL+11)
#define DEHUMIDIFIER_ACTUATOR_CHANGE (_PULSE_CODE_MINAVAIL+12)

/*********************************
 *  ACTUATOR CONSTANTS
 *********************************/
/*ACTUATOR STATES*/
#define ON 1
#define OFF 2

/*********************************
 *  SENSOR TRHRESHOLDS
 *********************************/
#define MAX_TEMP 25
#define MIN_TEMP 20


#endif /* SRC_CONSTANTS_H_ */
