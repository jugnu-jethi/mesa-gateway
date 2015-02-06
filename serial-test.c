#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>



#define FALSE ( 0 )
#define TRUE ( unsigned )( !FALSE )



int main( void ){
  
  int collectorfd = -1;
  
  collectorfd = open( "/dev/ttyUSB0", O_RDWR | O_NOCTTY );
  if( collectorfd < 0 ){
    perror( "open-collector" ); 
    exit( EXIT_FAILURE );
  }
  
  close( collectorfd );
  if( collectorfd < 0 ){
    perror( "close-collector" ); 
    exit( EXIT_FAILURE );
  }
  
  return EXIT_SUCCESS;
}