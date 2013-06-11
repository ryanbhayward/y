#ifndef CONNECT_H
#define CONNECT_H
#include "board.h"

//   win iff some stone-group touches all 3 borders so ...
//   ... for each stone-group, maintain set of touched borders

int Find(std::vector<int>& Parents, int x) ;
int Union(std::vector<int>& Parents, int x, int y) ;
#endif
