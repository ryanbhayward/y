#ifndef MOVE_H
#define MOVE_H
#include "board.h"

static const int NumNbrs = 6;              // num nbrs of each cell
extern const int Nbr_offsets[NumNbrs+1] ;  // last = 1st to avoid using %mod
extern const int Bridge_offsets[NumNbrs] ;

struct Move {
  int s;
  int lcn;
  Move(int x, int y) : s(x), lcn(y) {}
  Move() {}
} ;

bool has_win(int bd_set) ;
#endif
