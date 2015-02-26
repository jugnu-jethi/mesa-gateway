#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
  
#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyUSB1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define TMPCTRVAL 100
  
int STOP=FALSE; 
  
// void signal_handler_IO( int status );   /* definition of signal handler */
void sigact_handler_IO( int, siginfo_t *, void * );   /* definition of sigaction-signal handler */
volatile sig_atomic_t wait_flag=TRUE;                    /* TRUE while no signal received */
  
int main( void )
{
  int fd, c, res, writeOctets, tmpctr = TMPCTRVAL, sent = FALSE, totalRead = 0, totalSent = 0;
  struct termios oldtio,newtio;
  struct sigaction saio;           /* definition of signal action */
  char buf[255];
  const char wTest[] = "TESTING ONE TWO THREE FOUR FIVE SIX SEVEN EIGHT NINE TEN";
  
  /* open the device to be non-blocking (read will return immediatly) */
  fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd <0) {perror(MODEMDEVICE); exit(-1); }
  
  /* install the signal handler before making the device asynchronous */
//   saio.sa_handler = signal_handler_IO;
  saio.sa_sigaction = sigact_handler_IO;
  sigemptyset(&saio.sa_mask);
  saio.sa_flags = SA_SIGINFO | SA_RESTART;
  saio.sa_restorer = NULL;
  sigaction( SIGIO, &saio, NULL );
    
  /* allow the process to receive SIGIO */
  fcntl(fd, F_SETOWN, getpid());
  /* Make the file descriptor asynchronous (the manual page says only 
    O_APPEND and O_NONBLOCK, will work with F_SETFL...) */
  fcntl(fd, F_SETFL, FASYNC);
  
  tcgetattr(fd,&oldtio); /* save current port settings */
  /* set new port settings for canonical input processing */
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = 0;
  newtio.c_oflag &= ~OPOST; // raw output mode
  newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // raw input mode
  newtio.c_cc[ VMIN ]=0;
  newtio.c_cc[ VTIME ]=0;
  tcflush( fd, TCIFLUSH );
  tcsetattr( fd,TCSANOW,&newtio );
  
  /* loop while waiting for input. normally we would do something
    useful here */ 
  while( STOP == FALSE ){
    printf(".\n");
    /* after receiving SIGIO, wait_flag = FALSE, input is available
      and can be read */
    if( sent ==  FALSE ){
      if( -1 == ( writeOctets = write( fd, wTest, sizeof( wTest ) ) ) ){
	perror( "send-error" );
      }else{
	totalSent += writeOctets;
	printf("sent:%d\n", writeOctets );
	if( ( --tmpctr ) == 0 ) sent = TRUE;
      }
    }

    usleep(100000);
    
    if( wait_flag == FALSE ){ 
      if( -1 != ( res = read( fd, buf, 255 ) ) ){
	totalRead += res;
	buf[res]=0;
	printf("recvd:%s:%d:%d:%d\n", buf, res, totalRead, totalSent );
//      if (res==1) STOP=TRUE; /* stop loop if only a CR was input *
      }
      wait_flag = TRUE;      /* wait for new input */
    }
  }
  /* restore old port settings */
  tcsetattr(fd,TCSANOW,&oldtio);
  
  return EXIT_FAILURE;
}
  
/***************************************************************************
* signal handler. sets wait_flag to FALSE, to indicate above loop that     *
* characters have been received.                                           *
***************************************************************************/
  
// void signal_handler_IO( int status )
// {
//   printf("received SIGIO signal.\n");
//   wait_flag = FALSE;
// }

void sigact_handler_IO( int signalnum, siginfo_t *signalinfo, void *signalcontext )
{
  /* psiginfo() & psignal are UNSAFE; NOT one of async-signal-safe functions */
//   psiginfo( signalinfo, "SIGIO" );
//   psignal( signalnum, "SIGIO" );
  wait_flag = FALSE;
}
