/*
 * temperature_sensor.c
 *
 *	Multi-threaded process that pulls data from the read end of the pipe queue and stores the data point.
 *	Another thread waits on Client messages for data and returns the data
 *
 *  Created on: Nov. 18, 2022
 *      Author: benmask
 */
#include <stdio.h>
#include <constants.h>

void runServer();
void readData();

int main(void) {
	printf("Starting temp sensor");
	float tempData;

	/*Get the fd from the args passed in*/

	/*Init a mutex for the data*/

	/*Spawn readData thread*/

	/*Spawn the runServer thread*/

	/*wait on the threads*/
}

