#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
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
void * readRxRingBuffer( void * );



/* Global variables */
unsigned int HALT = FALSE;
RingBuffer collectorReadRB;
ElemType serialReadData, bufferReadData;
pthread_mutex_t RxRBmutex;
sem_t RxRBCount;



int main( void ){
  
  int collectorfd = -1;
  unsigned int writeOctets = 0, readOctets = 0, tmpctr;
  char collectorbuffer[ COLLECTOR_BUFFER_SZ ];
  pthread_attr_t tmpattr;
  pthread_t readRxRBTh;
  
  struct termios collector_tty_options, prev_tty_options;
  
  /* all variables init to known state i.e. zero, etc */ 
  memset( &collector_tty_options, 0, sizeof( collector_tty_options ) );
  memset( &prev_tty_options, 0, sizeof( collector_tty_options ) );
  memset( collectorbuffer, NUL_CHAR, sizeof( collectorbuffer ) );
  
  /* init RxRB mutex */
  pthread_mutex_init( &RxRBmutex, NULL );
  
  /* initialise RxRB semaphore */
  sem_init( &RxRBCount, 0, 0 );
  
  /* initialise a receive ring buffer */
  rbInit( &collectorReadRB, READ_RING_BUFFER_SZ );  
  
  /* raw mode, non-canonical processing */
  /* vmin==0; vtime==0;; read() returns bytes requested or lesser */
  collector_tty_options.c_cc[ VMIN ] = 1;
  collector_tty_options.c_cc[ VTIME ] = 0;
  collector_tty_options.c_cflag &= ~( CSIZE | PARENB );
  collector_tty_options.c_cflag |= BAUDRATE | CS8 | CLOCAL | CREAD;
  collector_tty_options.c_iflag &= ~( IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | IXON );
  collector_tty_options.c_iflag |= ICRNL;
  /* raw input mode */
  collector_tty_options.c_lflag &= ~( ECHO | ECHONL | ICANON | ISIG | IEXTEN );
  /* raw output mode */
  collector_tty_options.c_oflag &= ~OPOST;
  
  /* open collector serial port with exit error prompt */
  collectorfd = open( COLLECTOR_TTY, O_RDWR | O_NOCTTY );
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
  
  /* Create a detached state thread and clean-up thread attrib after */
  pthread_attr_init( &tmpattr );
  pthread_attr_setdetachstate( &tmpattr, PTHREAD_CREATE_DETACHED );
  if( 0 != pthread_create( &readRxRBTh, &tmpattr, readRxRingBuffer, NULL ) ){
    
    perror( "ring-buffer-read-thread-create-error" );
    
    if( 0 > close( collectorfd ) ){
      
      perror( "close-collector" ); 
      
    }
    
    exit( EXIT_FAILURE );
    
  }
  pthread_attr_destroy( &tmpattr );
  
  /* start main program loop */
  while( FALSE == HALT ){
    
    /* receive from collector tty */
    if( 0 > ( readOctets = read( collectorfd, collectorbuffer, READ_REQUEST_SZ ) ) ){
      
      perror( "read-error" );
      
    }
    
    /* store read collector data into ring buffer */
    if( 0 < readOctets ){
      
      printf( "read-tty: %dB - ", readOctets );
      
      /* Lock RxRB for writing */
      pthread_mutex_lock( &RxRBmutex ); 
        
      for( tmpctr = 0; tmpctr < readOctets; tmpctr++ ){
        
        printf( "%x", collectorbuffer[ tmpctr ] );
        serialReadData.data = collectorbuffer[ tmpctr ];
        rbWrite( &collectorReadRB, &serialReadData );
//         printf( "Serial-data-stored.\n" );
        
        /* Increment semaphore to indicate data availability */
        sem_post( &RxRBCount );
        
      }
      
      /* Unlock RxRB after writing */
      pthread_mutex_unlock( &RxRBmutex );
        
      printf( "\n" );
      
//       /* Lock RxRB for reading */
//       pthread_mutex_lock( &RxRBmutex );
//       
//       /* read and print stored ring buffer data */
//       while( !rbIsEmpty( &collectorReadRB ) ){
//         
//         printf( "read-buffer: " );
//         
//         rbRead( &collectorReadRB, &bufferReadData );
//         printf( "%x", bufferReadData.data );
//         
//       }
//       
//       printf( "\n" );
//       
//       /* Unlock RxRB after writing */
//       pthread_mutex_unlock( &RxRBmutex );
        
      /* reinitialise variables and free allocations for next iteration use */
      readOctets = 0;
      memset( collectorbuffer, NUL_CHAR, sizeof( collectorbuffer ) );
      
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



void * readRxRingBuffer( void *threadArg ){
  
  unsigned int printctr, retvar;
  
  
  
  while( 1 ){
    
    /* Wait for data to become available */
    sem_wait( &RxRBCount );
    
    if( 0 == sem_getvalue( &RxRBCount, &retvar ) ){
      
      printf( "Semaphore-value: %d\n", retvar );
      
    }else{
      
      perror( "semaphore-error" );
      
    }
    
    /* Lock RxRB for reading */
      pthread_mutex_lock( &RxRBmutex );
      
      
      printf( "read-buffer: " );
      
      /* read and print stored ring buffer data */
      if( !rbIsEmpty( &collectorReadRB ) ){
        
        rbRead( &collectorReadRB, &bufferReadData );
        printf( "%x", bufferReadData.data );
        
      }
      
      printf( "\n" );
      
      /* Unlock RxRB after writing */
      pthread_mutex_unlock( &RxRBmutex );

  }

}
  