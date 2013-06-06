#ifndef GENMOVE_H
#define GENMOVE_H
#include "board.h"

static const int ROLLOUTS = 100000;

int rand_move  (Board&) ;
int monte_carlo(Board&, int s, bool useMiai, bool accelerate) ;
int uct_move   (Board&, int s, bool useMiai) ;
#endif
