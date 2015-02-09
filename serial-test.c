#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>



#define FALSE ( 0 )
#define TRUE ( unsigned )( !FALSE )

#define COLLECTOR_TTY "/dev/ttyUSB0"
#define MSCAN_FRAME_MAX_SZ 268
#define COLLECTOR_BUFFER_SZ ( 1 * MSCAN_FRAME_MAX_SZ )



int main( void ){
  
  int collectorfd = -1, writeOctets = 0, readOctets = 0;
  char collectorbuffer[ COLLECTOR_BUFFER_SZ ];
  char wTest[] = "TESTING ONE TWO THREE!";
  struct termios collector_tty_options;
  
  memset( &collector_tty_options, 0, sizeof( collector_tty_options ) );
  memset( collectorbuffer, 0, sizeof( collectorbuffer ) );
  collector_tty_options.c_iflag = 0;
  collector_tty_options.c_oflag = 0;
  collector_tty_options.c_cflag = CS8 | CREAD | CLOCAL;           // 8n1, see termios.h for more information
  collector_tty_options.c_lflag = 0;
  collector_tty_options.c_cc[ VMIN ] = 1;
  collector_tty_options.c_cc[ VTIME ] = 5;
  
  /* open collector serial port with exit error prompt */
  collectorfd = open( COLLECTOR_TTY, O_RDWR | O_NOCTTY );
  if( collectorfd < 0 ){
    perror( "open-collector" ); 
    exit( EXIT_FAILURE );
  }

    cfsetospeed( &collector_tty_options, B115200 );
    cfsetispeed( &collector_tty_options, B115200 );
    
  if( tcflush( collectorfd, TCIOFLUSH ) < 0 ){
    perror( "flush-collector-IO" );
  }
  if( tcsetattr( collectorfd, TCSANOW, &collector_tty_options ) < 0 ){
    perror( "set-collector-tty-options" );
    if( close( collectorfd ) < 0 ){
      perror( "close-collector" ); 
    }
    exit( EXIT_FAILURE );
  }
  
  /* send "TEST" message */
  writeOctets = write( collectorfd, wTest, sizeof( wTest ) );
  printf( "wrote %d bytes\n", writeOctets );
  
  usleep( 15625 );
  
  /* receive "TEST" message. connect loopback plug */
  if( ( readOctets = read( collectorfd, collectorbuffer, MSCAN_FRAME_MAX_SZ ) ) < 0 ){
    perror( "read-error" );
  }
  printf( "read %d bytes - %s\n", readOctets, collectorbuffer );
  
  /* close collector serial port with exit error prompt */
  if( close( collectorfd ) < 0 ){
    perror( "close-collector" ); 
    exit( EXIT_FAILURE );
  }
  
  return EXIT_SUCCESS;
}