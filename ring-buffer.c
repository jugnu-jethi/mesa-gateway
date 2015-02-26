/* Circular buffer - keeps one slot open */ 
#include <stdlib.h>

#include "ring-buffer.h"



void rbInit( RingBuffer *rb, unsigned int size ){
  
    rb->size  = size + 1; /* include empty elem */
    rb->start = 0;
    rb->end   = 0;
    rb->elems = ( ElemType * )calloc( rb->size, sizeof( ElemType ) );
    
}
 
void rbFree( RingBuffer *rb ){
  
    free(rb->elems); /* OK if null */ 
  
}
 
int rbIsFull( RingBuffer *rb ){
  
    return ( rb->end + 1 ) % rb->size == rb->start; 
  
}
 
int rbIsEmpty( RingBuffer *rb ){
  
    return rb->end == rb->start; 
  
}
 
/* Write an element, overwriting oldest element if buffer is full. 
 Avoid the overwrite by checking rbIsFull(). */
void rbWrite( RingBuffer *rb, ElemType *elem ){
  
    rb->elems[ rb->end ] = *elem;
    rb->end = ( rb->end + 1 ) % rb->size;
    
    if ( rb->end == rb->start ){
      rb->start = ( rb->start + 1 ) % rb->size; /* full, overwrite */
    }

}
 
/* Read oldest element. App must ensure !rbIsEmpty() first. */
void rbRead( RingBuffer *rb, ElemType *elem ){
  
    *elem = rb->elems[ rb->start ];
    rb->start = ( rb->start + 1 ) % rb->size;
    
}


