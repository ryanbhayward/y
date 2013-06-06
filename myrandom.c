#include <stdio.h>
#include "board.y.h"
#include "myrandom.h"

void swap_int(int *x, int *y) { 
  int tmp = *x; *x = *y; *y = tmp;
}

//shuffle vector interval indexed a..b
void shuffle_interval(int X[], int a, int b) {
  int j,k;
  for (k = b; k > a; k--) {
    j = myrand(k+1-a);  //printf("rand %d\n", j);
    swap_int(&X[j],&X[k]);
  }
}

