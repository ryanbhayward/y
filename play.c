//   play y               RBH 2012 ... tips from Jakub
#include <stdio.h>
#include <stdlib.h>
#include "board.y.h"
#include "connect.h"
#include "interact.h"

int main(void) { struct myboard myB;
  interact(&myB);
  return 0;
}
