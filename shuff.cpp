#include <cstdio>
#include "shuff.h"

//shuffle vector interval indexed a..b
void shuffle_interval(int X[], int a, int b) {
  int j,k;
  //for (k = b; k > a; k--) {
    //j = myrand(k+1-a);  //printf("rand %d\n", j);
    //swap(X[j],X[k]);
  //}
  for (k = a+1; k <= b; k++) {
    j = myrand(a,k);  
    swap(X[j],X[k]);
  }
}

