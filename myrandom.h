#ifndef MYRANDOM_H
#define MYRANDOM_H
#include <stdlib.h>

#define myrand(n) (rand()%(n))     // pseudo-uniform-random in [0.. n-1]

extern void shuffle_interval(int X[], int a, int b) ;
extern void swap_int(int *x, int *y) ;
#endif
