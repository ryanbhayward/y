//   play y               RBH 2012 ... tips from Jakub
#include <cstdio>
#include <cstdlib>
#include "board.h"
#include "connect.h"
#include "interact.h"

int main(void) {Board myB; 
  shapeAs(TRI,myB.brdr);
  display_nearedges();
  interact(myB);
  return 0;
}
