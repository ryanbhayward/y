#ifndef CONNECT_H
#define CONNECT_H
#include "board.y.h"

//   win iff some stone-group touches all 3 borders so ...
//   ... for each stone-group, maintain set of touched borders

extern int my_find(int Parents[TotalGBCells], int x) ;
extern int my_union(int Parents[TotalGBCells], int x, int y) ;
#endif
