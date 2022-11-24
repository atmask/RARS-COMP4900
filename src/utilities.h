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
#include <stdarg.h>

#define MAX_STRING 512

int logString(FILE* logFile, char* s, ...)
{
	time_t ltime; /* calendar time */
	ltime=time(NULL); /* get current cal time */
	char *timeString = asctime(localtime(&ltime));
	timeString[24] = '\0';

	char buffer[MAX_STRING];
	va_list args;
	va_start(args, s);
	vsnprintf(buffer, MAX_STRING, s, args);

	fprintf(logFile, "%s: %s\n",timeString, buffer);
	fflush(logFile);
	va_end(args);
	return 0;
}
