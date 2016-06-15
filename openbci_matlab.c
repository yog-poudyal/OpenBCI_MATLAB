/*

This program provides serial port communication with the OpenBCI Board.

NOTES:
-OpenBCI board uses asynchronous, 

*/

#include <stdio.h>			 		// standard input/output definitions
#include <termios.h>				// POSIX terminal control definitions
#include <fcntl.h>					// File Control definitions
#include <unistd.h>					// UNIX standard function definitions
#include <errno.h>					//
#include <sys/signal.h>
#include <sys/types.h>

#define BAUDRATE B115200			// define baudrate (115200bps)
#define PORT "/dev/ttyUSB0"			// define port
#define _POSIX_SOURCE 1				// POSIX compliant source ((((how necessary is this...))))
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE; 

void signal_handler_IO (int status);   /* definition of signal handler */
float byte_parser (unsigned char buf[], int res);
int wait_flag=TRUE;    

main()
{
	int fd;								// File descriptor for the port
	int c;
	int res;
	unsigned char buf[1024];
	struct termios serialportsettings;  // Declare the serial port struct
	struct sigaction saio;				// Declare signals
	fd = open(PORT, O_RDWR | O_NOCTTY); // declare serial port file descriptor

	//*****************************************************************************************
	//	SERIAL PORT SETUP 

	// Attempt to read attributes of serial port
	if(tcgetattr(fd,&serialportsettings) != 0)
		printf("\n ERROR! In opening ttyUSB0\n");
	else
		printf("\n ttyUSB0 Opened Successfully\n");
	
	//Baud Rate information (Baud=115200)
	cfsetispeed(&serialportsettings,B115200);		// set the input baud rate
	cfsetospeed(&serialportsettings,B115200);		// set the output baud rate	

	//Hardware Information
	serialportsettings.c_cflag &= ~PARENB;			// set the parity bit (0)
	serialportsettings.c_cflag &= ~CSTOPB;			// stop bits = 1
	serialportsettings.c_cflag &= ~CSIZE;			// clears the mask
	serialportsettings.c_cflag |= CS8;				// set the data bits = 8
	serialportsettings.c_cflag &= ~CRTSCTS;			// turn off hardware based flow control (RTS/CTS)
	serialportsettings.c_cflag |= CREAD | CLOCAL;	// turn on the receiver of the serial port (CREAD)
	//Input data flags (not needed?)
	// serialportsettings.c_iflag &= ~(IXON | IXOFF | IXANY);
	//Echoing and character processing flags
	serialportsettings.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); //no echoing, input proc, signals, or background proc halting
	//Output data flags ("for ")
	// serialportsettings.c_oflag |= OPOST; //causes the output data to be processed in an implementation-defined manner

	tcsetattr(fd,TCSANOW,&serialportsettings); 		//set the above attributes


	/* install the signal handler before making the device asynchronous
		sa_handler = pointer to the signal-catching function
		sa_mask = additional set of signals to be blocked during execution of signal-catching fxn
		sa_flags = special flags to affect behavior of signal */

	saio.sa_handler = signal_handler_IO;
	// saio.sa_mask = 0;		\\this kept giving me an error, so the next two lines replace this
	sigemptyset(&saio.sa_mask);
	sigaddset(&saio.sa_mask, SIGINT);
	saio.sa_flags = 0;
	saio.sa_restorer = NULL;
	sigaction(SIGIO,&saio,NULL);

	/* Make the file descriptor asynchronous (the manual page says only 
	   O_APPEND and O_NONBLOCK, will work with F_SETFL...) */
	// fcntl(fd, F_SETFL, FASYNC);
	fcntl(fd, F_SETFL, FNDELAY|O_ASYNC );

	/* Special Characters
		MIN > 0, TIME == 0 (blocking read)
		  read(2) blocks until MIN bytes are available, and returns up
		  to the number of bytes requested.

		VMIN = minimal number of characters for noncanonical read
		VTIME = timeout in deciseconds for noncanonical read */
	serialportsettings.c_cc[VMIN]=33;
	serialportsettings.c_cc[VTIME]=0;

	/* Flush
		fd = object
		TCIOFLUSH = flush written and received data */
	tcflush(fd, TCIOFLUSH);

	/* Sets parameters associated with the board
		fd = object
		TCSANOW = the change occurs immediately
		&serialportsettings = pointer to serialportsettings */
	tcsetattr(fd,TCSANOW,&serialportsettings);


	while (STOP==FALSE) {
		if (wait_flag==FALSE) { 
			int bytes_available;
			// ioctl(fd, FIONREAD, &bytes_available);
			// printf("BYTES WAITING: %d", FIONREAD);
			res = read(fd,&buf,33);
			byte_parser(buf,res);
			// printf("%s\n", buf);
			printf(":number of bytes:%d\n", res);
			if (res==0){
				STOP=TRUE;		// stop loop if only a CR was input 
				printf("STOPPED STREAM\n");
			}
			wait_flag = TRUE;			/* wait for new input */
		}
	}

	close(fd);
}

void signal_handler_IO (int status){
	// printf("-------------------received SIGIO signal---------------------------\n");
	wait_flag = FALSE;
}

float byte_parser (unsigned char buf[], int res){
	for (int i=0; i<res;i++){
		printf("BYTE %d\n",buf[i]);
	}
	// printf("FIRST BYTE %i\n",buf[0]);
	// if (buf[0] == 192){
	// 	printf("yessssss\n");
	// 	sleep(2);
	// }
	// else
	// 	printf("noooooo\n");
	return 0.0;
}