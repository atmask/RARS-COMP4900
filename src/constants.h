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


/*******************
 * SENSOR SERVERS
 *******************/

// Server names
#define TEMPERATURE_SERVER "temperature_sensor"
#define DISPLAY_SERVER "display"


// Message types
#define GET_DATA (_IO_MAX+1)
#define SENSOR_DATA (_IO_MAX+2)


// Message structs to ask for temp data
typedef struct get_snsr_data_msg {
	uint16_t type;
} get_snsr_data__msg_t;

typedef struct resp_snsr_data_msg {
	float		data;
} resp_snsr_data_msg_t;



/*******************
 *  PULSE CODES
 *******************/
#define TEMP_DATA (_PULSE_CODE_MINAVAIL+1)


/*Actuator Constants*/
#define UP 1
#define DOWN 2
#define OFF 3

/*SENSOR TYPES*/
#define TEMPERATURE 1


#endif /* SRC_CONSTANTS_H_ */
