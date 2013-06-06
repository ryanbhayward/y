#include <stdio.h>
#include <assert.h>
#include "connect.h"
#include "move.h"
#include "myrandom.h"

Bool_t has_win(int bd_set, int stn) {
  return (bd_set==BORDER_ALL) ? TRUE:FALSE;
}

Bool_t not_in_miai(int A[], int lcn) { 
  return (A[lcn]==lcn) ? TRUE:FALSE; 
}

void set_miai(struct myboard *M, int s, int x, int y) { 
  M->reply[ndx(s)][x] = y; 
  M->reply[ndx(s)][y] = x; 
}

void release_miai(struct myboard *M, int s, int x) { 
  int y = M->reply[ndx(s)][x]; 
  M->reply[ndx(s)][x] = x; 
  M->reply[ndx(s)][y] = y; 
}

void put_stone(int stone, int lcn, int A[]) {
  assert(A[lcn]==EMPTY_CELL); A[lcn] = stone;
}

void remove_stone(int lcn, int A[]) {
  assert(A[lcn]!=EMPTY_CELL); A[lcn] = EMPTY_CELL;
}

// put stone on board, and update own connectivity..
//   this move can break oppt's miai-connectivity if oppt does not respond
//   using_miai ? bridge adjacency : stone adjacency
int make_move(int stone, int lcn, struct myboard* B, Bool_t using_miai) { 
  int t,nbr,nbrRoot,captain; 
  //show_Yboard(B->board);show_miai(B->reply[0]);
  //show_miai(B->reply[1]); printf("next move %d\n",lcn);
  if(B->board[lcn]!=EMPTY_CELL) 
    printf("make_move %c %c%d fails\n",emit(stone),alphaCol(lcn),numerRow(lcn));
  assert(B->board[lcn]==EMPTY_CELL); assert(B->brdr[lcn]==BORDER_NONE);
  put_stone(stone,lcn,B->board);
  captain = lcn; // captain of stone group containing lcn
  for (t=0; t<NumNbrs; t++) {
    nbr = lcn + Nbr_origin_offsets[t];
    if (B->board[nbr] == stone) {
      nbrRoot = my_find(B->p,nbr);
      B->brdr[nbrRoot] |= B->brdr[captain];
      captain = my_union(B->p,captain,nbrRoot); } 
    else if (B->board[nbr] == GUARD_CELL) {
      B->brdr[captain] |= B->brdr[nbr]; }
  }
  if (!using_miai) return B->brdr[captain];  
  // else continue... 
  release_miai(B,stone,lcn); // your own miai, so connectivity ok
  // avoid directional bridge bias: search in random order
  int x, perm[NumNbrs] = {0, 1, 2, 3, 4, 5}; 
  shuffle_interval(perm,0,NumNbrs-1); 
  for (t=0; t<NumNbrs; t++) {
    x = perm[t];
    assert((x>=0)&&(x<NumNbrs));
    nbr = lcn + Bridge_origin_offsets[x]; // nbr via bridge
    int c1 = lcn+Nbr_origin_offsets[x];     // carrier 
    int c2 = lcn+Nbr_origin_offsets[x+1];   // other carrier
    if ((B->board[nbr] == stone)&&
        (B->board[c1] == EMPTY_CELL)&&
        (B->board[c2] == EMPTY_CELL)&&
        (not_in_miai(B->reply[ndx(stone)],c1))&&  
        (not_in_miai(B->reply[ndx(stone)],c2))&&  
        (my_find(B->p,nbr)!=my_find(B->p,lcn))) {  // new miai
      //printf("miai lcn %2d c1 %2d c2 %2d\n",nbr,c1,c2);
      nbrRoot = my_find(B->p,nbr);
      B->brdr[nbrRoot] |= B->brdr[captain];
      captain = my_union(B->p,captain,nbrRoot); 
      set_miai(B,stone,c1,c2);
    } 
    else if ((B->board[nbr] == GUARD_CELL)&&
             (B->board[c1] == EMPTY_CELL) &&
             (B->board[c2] == EMPTY_CELL)&&
             (not_in_miai(B->reply[ndx(stone)],c1))&&
             (not_in_miai(B->reply[ndx(stone)],c2))) { // new miai
      printf("border miai lcn %2d c1 %2d c2 %2d\n",nbr,c1,c2);
      B->brdr[captain] |= B->brdr[nbr];
      set_miai(B,stone,c1,c2);
      }
  }
  return B->brdr[captain];
}
