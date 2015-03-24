#ifndef RING_BUFFER_H
#define RING_BUFFER_H



/* Opaque buffer element type */
typedef struct {
  
  unsigned char data;
  
} ElemType;

/* Ring buffer object */
typedef struct {
  
  unsigned int         size;   /* maximum number of elements           */
  unsigned int         start;  /* index of oldest element              */
  unsigned int         end;    /* index at which to write new element  */
  ElemType   *elems;  /* vector of elements */
  
} RingBuffer;



/* Initialise an empty circular buffer of size unsigned int. */
void rbInit( RingBuffer *, unsigned int );

/* Free allocation of a circular buffer i.e. destroy */
void rbFree( RingBuffer * );

/* Circular buffer full check */
int rbIsFull( RingBuffer * );

/* Circular buffer empty check */
int rbIsEmpty( RingBuffer * );

/* Write data of ElemType to circular buffer */
void rbWrite( RingBuffer *, ElemType * );

/* Read data of ElemType from circular buffer */
void rbRead( RingBuffer *, ElemType * );



#endif