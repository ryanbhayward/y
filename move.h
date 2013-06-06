#ifndef MOVE_H
#define MOVE_H
#include "board.y.h"

extern int make_move(int stone, int lcn, struct myboard*, Bool_t) ;
extern Bool_t has_win(int bd_set, int stn) ;
void set_miai(struct myboard *M, int s, int x, int y) ;
void release_miai(struct myboard *M, int s, int x) ;
void put_stone(int stone, int lcn, int A[]) ;
void remove_stone(int lcn, int A[]) ;
#endif
