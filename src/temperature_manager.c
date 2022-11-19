#include <stdio.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <stdlib.h>
#include <constants.h>

int main(int argc, char **argv){
	printf("TempManager\n");

	/*
	typedef union
	{
		struct _pulse pulse;
	    char rmsg [MAX_STRING_LEN +1];
	} myMessage_t;
	myMessage_t msg;
	*/

	int chid, rcvid;
	name_attach_t* attach;
	attach = name_attach(NULL, "TempMan", 0);

	//create a channel
	chid = ChannelCreate(0);


	//spawn thermometer

	//spawn drivers (AC, Heater)


	/*while(1){

		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);

	 }*/
	return EXIT_SUCCESS;
}
