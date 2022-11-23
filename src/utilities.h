/*
 * utilities.h
 *
 *	Various utility functions used by multiple processes
 *
 *  Created on: Nov. 22, 2022
 */

/*
 * Print string to log along with timestamp
 */

#include <stdio.h>
#include <time.h>

int logString(FILE* logFile, char* s)
{
	time_t ltime; /* calendar time */
	ltime=time(NULL); /* get current cal time */
	fprintf(logFile, "%s: %s\n",asctime(localtime(&ltime)), s);
	return 0;
}
