#ifndef MYRANDOM_H
#define MYRANDOM_H
#include <cstdlib>

void shuffle_interval(int X[], int a, int b) ;

static inline int myrand(int x, int y) {return x+rand()%(y-x);}  // in [x.. y-1] 
#endif
