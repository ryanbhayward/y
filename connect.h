#ifndef CONNECT_H
#define CONNECT_H
#include "board.h"

//   win iff some stone-group touches all 3 borders so ...
//   ... for each stone-group, maintain set of touched borders

int Find(int Parents[TotalGBCells], int x) ;
int Union(int Parents[TotalGBCells], int x, int y) ;
#endif
