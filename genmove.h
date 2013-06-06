#ifndef GENMOVE_H
#define GENMOVE_H
#include "board.y.h"

#define ROLLOUTS 3

extern int rand_move(int G[]) ;
extern int monte_carlo(int who, struct myboard*, Bool_t) ;
#endif
