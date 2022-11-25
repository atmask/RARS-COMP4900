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
#define TEMPERATURE_SERVER "temperature_sensor"
#define DISPLAY_SERVER "display"
#define TEMPERATURE_SENSOR_SERVER "temperature_sensor"
#define TEMPERATURE_ACTUATOR_SERVER "temperature_actuator"
#define HEATER_ACTUATOR_SERVER "heater_actuator"
#define AIR_CONDITIONER_ACTUATOR_SERVER "air_conditioner_actuator"

/**********************************************
 * MESSAGE TYPES
 ********************************************/
#define GET_DATA (_IO_MAX+1)
#define SENSOR_DATA (_IO_MAX+2)
#define COMMAND_ACTUATOR_STATE (_IO_MAX+3)
#define ACTUATOR_STATE (_IO_MAX+4)

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
#define TEMP_DATA (_PULSE_CODE_MINAVAIL+1)


/*********************************
 *  ACTUATOR CONSTANTS
 *********************************/
#define UP 1
#define DOWN 2

#define ON 1
#define OFF 3

/*SENSOR TYPES*/
#define TEMPERATURE 1


/*********************************
 *  SENSOR TRHRESHOLDS
 *********************************/
#define MAX_TEMP 25
#define MIN_TEMP 20


#endif /* SRC_CONSTANTS_H_ */
