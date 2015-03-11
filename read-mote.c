#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ring-buffer.h"



/* User definitions */
#define FALSE ( 0 )
#define TRUE ( unsigned )( !FALSE )
#define NUL_CHAR '\0'

#define BAUDRATE B38400
#define COLLECTOR_TTY "/dev/ttyUSB0"
#define MSCAN_FRAME_MAX_SZ 268
#define COLLECTOR_BUFFER_SZ ( 1 * MSCAN_FRAME_MAX_SZ )
#define READ_REQUEST_SZ ( 0.25 * MSCAN_FRAME_MAX_SZ )
#define READ_RING_BUFFER_SZ ( 10 * MSCAN_FRAME_MAX_SZ )



/* Function prototypes */
void sigact_handler_IO( int, siginfo_t *, void * );



/* Global variables */
unsigned int HALT = FALSE;
volatile sig_atomic_t readFlag = FALSE;



int main( void ){
  
  int collectorfd = -1;
  unsigned int writeOctets = 0, readOctets = 0, HALT = FALSE, tmpctr;
  char collectorbuffer[ COLLECTOR_BUFFER_SZ ];
  ElemType serialReadData = { 0, NULL }, bufferReadData = { 0, NULL };
  RingBuffer collectorReadRB;
  
  struct sigaction signal_IO_options;
  struct termios collector_tty_options, prev_tty_options;
  
  /* all variables init to known state i.e. zero, etc */ 
  memset( &collector_tty_options, 0, sizeof( collector_tty_options ) );
  memset( &prev_tty_options, 0, sizeof( collector_tty_options ) );
  memset( collectorbuffer, NUL_CHAR, sizeof( collectorbuffer ) );
  
  /* initialise a receive ring buffer */
  rbInit( &collectorReadRB, READ_RING_BUFFER_SZ );
  
  /* vmin==0; vtime==0;; read() returns bytes requested or lesser */
  collector_tty_options.c_cc[ VMIN ] = 0;
  collector_tty_options.c_cc[ VTIME ] = 0;
  collector_tty_options.c_cflag &= ~( CSIZE | PARENB );
  collector_tty_options.c_cflag |= BAUDRATE | CS8 | CLOCAL | CREAD;
  collector_tty_options.c_iflag &= ~( IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | IXON );
  collector_tty_options.c_iflag |= ICRNL;
  /* raw input mode */
  collector_tty_options.c_lflag &= ~( ECHO | ECHONL | ICANON | ISIG | IEXTEN );
  /* raw output mode */
  collector_tty_options.c_oflag &= ~OPOST;

  /* install the signal handler before making the device asynchronous */
  signal_IO_options.sa_sigaction = sigact_handler_IO;
  sigemptyset(&signal_IO_options.sa_mask);
  signal_IO_options.sa_flags = SA_SIGINFO | SA_RESTART;
  signal_IO_options.sa_restorer = NULL;
  sigaction( SIGIO, &signal_IO_options, NULL );
  
  /* open collector serial port with exit error prompt */
  collectorfd = open( COLLECTOR_TTY, O_RDWR | O_NOCTTY | O_NONBLOCK );
  if( 0 > collectorfd ){
    
    perror( "open-collector" ); 
    exit( EXIT_FAILURE );
    
  }
  
  /* save previous tty option for the collector device */
  if( 0 > tcgetattr( collectorfd, &prev_tty_options ) ){
    
    perror( "get-prev-tty-options" );
    
    if( close( collectorfd ) < 0 ){
      
      perror( "close-collector" );
      
    }
    
    exit( EXIT_FAILURE );
    
  }
  
   /* flush all buffered io data */
  if( 0 > tcflush( collectorfd, TCIOFLUSH ) ){
    
    perror( "flush-collector-IO" );
    
  }
  
  /* init new collector tty options */
  if( 0 > tcsetattr( collectorfd, TCSANOW, &collector_tty_options ) ){
    
    perror( "set-collector-tty-options" );
    
    if( 0 > close( collectorfd ) ){
      
      perror( "close-collector" ); 
      
    }
    
    exit( EXIT_FAILURE );
    
  }
  
  /* allow the process to receive SIGIO */
  if( 0 > fcntl( collectorfd, F_SETOWN, getpid() ) ){
    
    perror( "set-recv-sigio" );
    
    if( 0 > close( collectorfd ) ){
      
      perror( "close-collector" ); 
      
    }
    
    exit( EXIT_FAILURE );
    
  }
  
  /* Make the file descriptor asynchronous */
  /* At success, SIGIO signal handler will trigger onwards */
  if( 0 > fcntl( collectorfd, F_SETFL, FASYNC ) ){
    
    perror( "set-async-descriptor" );
    
    if( 0 > close( collectorfd ) ){
      
      perror( "close-collector" ); 
      
    }
    
    exit( EXIT_FAILURE );
    
  }
  
  /* start main program loop */
  while( FALSE == HALT ){

    if( TRUE == readFlag ){
      
      /* receive from collector tty */
      if( 0 > ( readOctets = read( collectorfd, collectorbuffer, READ_REQUEST_SZ ) ) ){
        
	perror( "read-error" );
        
      }
      
      if( 0 < readOctets ){
        
	printf( "read %d bytes - ", readOctets );
        
	for( tmpctr = 0; tmpctr < readOctets; tmpctr++ ){
          
	  printf( "%x", collectorbuffer[ tmpctr ] );
          
	}
	
	/* store read collector data into ring buffer */
	serialReadData.data = ( unsigned char * )calloc( readOctets, sizeof( unsigned char ) );
        if( NULL != serialReadData.data ){
          
          if( serialReadData.data == memcpy( serialReadData.data, collectorbuffer, readOctets ) ){
            
            serialReadData.size = readOctets;
            rbWrite( &collectorReadRB, &serialReadData );
            
          }else{
            
            perror( "serial-data-copy-error" );
            
          }

        }else{
          
          perror( "ring-buffer-calloc-error" );
          
        }
        
        /* read and print stored ring buffer data */
        if( !rbIsEmpty( &collectorReadRB ) ){
          
          printf( " - " );
          rbRead( &collectorReadRB, &bufferReadData );
          
          for( tmpctr = 0; tmpctr < bufferReadData.size; tmpctr++ ){
          
            printf( "%x", bufferReadData.data[ tmpctr ] );
          
          }
          
        }
        
        if( 0 != memcmp( serialReadData.data, bufferReadData.data, sizeof( serialReadData.data ) ) ){
          
          printf( " - NOK!" );
          
        }else{
          
          printf( " - OK!" );
          
        }
        
        printf( "\n" );
        
        /* reinitialise variables and free allocations for next iteration use */
	readOctets = 0;
	memset( collectorbuffer, NUL_CHAR, sizeof( collectorbuffer ) );
        free( serialReadData.data );
        serialReadData.size = 0;
        
      }

      /* Potential race conditon */
      readFlag = FALSE;
    }
    
  }
  
  /* restore previous collector tty options */
  if( tcsetattr( collectorfd, TCSANOW, &prev_tty_options ) < 0 ){
    
    perror( "restore-collector-tty-options" );
    
  }

  /* close collector serial port with exit error prompt */
  if( close( collectorfd ) < 0 ){
    
    perror( "close-collector" ); 
    exit( EXIT_FAILURE );
    
  }
  
  return EXIT_SUCCESS;
}



/* SIGIO sigaction based signal handler */
void sigact_handler_IO( int signalnum, siginfo_t *signalinfo, void *signalcontext ){
  
  /* psiginfo() & psignal are UNSAFE; NOT one of async-signal-safe functions */
//   psiginfo( signalinfo, "SIGIO" );
  readFlag = TRUE;
  
}
