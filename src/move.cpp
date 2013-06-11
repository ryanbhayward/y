#include <stdio.h>
#include <cassert>
#include "board.h"
#include "connect.h"
#include "move.h"

bool has_win(int bd_set) { return bd_set==BRDR_ALL ; }
bool Board::not_in_miai(Move mv) { return reply[ndx(mv.s)][mv.lcn]==mv.lcn ; }

void Board::set_miai(int s, int x, int y) { 
  reply[ndx(s)][x] = y; 
  reply[ndx(s)][y] = x; }

void Board::release_miai(Move mv) { // WARNING: does not alter miai connectivity
  int y = reply[ndx(mv.s)][mv.lcn]; 
  reply[ndx(mv.s)][mv.lcn] = mv.lcn; 
  reply[ndx(mv.s)][y] = y; }

void Board::put_stone(Move mv) { 
  assert((board[mv.lcn]==EMP)||(board[mv.lcn]==mv.s)||board[mv.lcn]==TMP); 
  board[mv.lcn] = mv.s; }

void Board::YborderRealign(Move mv, int& cpt, int c1, int c2, int c3) {
  // no need to undo connectivity :)
  // realign Y border miai as shown  ... x = mv.lcn
  //   * * * *       * * * *
  //    2 1 y         . 1 y
  //     x             x 3
  assert(near_edge(c1) && near_edge(c2)); //printf("Y border realign %c",emit(s));
  release_miai(Move(mv.s,c1));
  set_miai(mv.s,c1,c3); assert(not_in_miai(Move(mv.s,c2)));
  //printf(" new miai"); prtLcn(c1); prtLcn(c3); show();
  int xRoot = Find(parent,mv.lcn);
  brdr[xRoot] |= brdr[cpt];
  cpt = Union(parent,cpt,xRoot);
}

int Board::moveMiaiPart(Move mv, bool useMiai, int& bdset, int cpt) {
// useMiai true... continue with move( )...
  int nbr,nbrRoot; int lcn = mv.lcn; int s = mv.s;
  release_miai(mv); // your own miai, so connectivity ok
  int miReply = reply[ndx(oppnt(s))][lcn];
  if (miReply != lcn) {
    //prtLcn(lcn); printf(" released opponent miai\n"); 
    release_miai(Move(oppnt(s),lcn));
  }
  // avoid directional bridge bias: search in random order
  int x, perm[NumNbrs] = {0, 1, 2, 3, 4, 5}; 
  //shuffle_interval(perm,0,NumNbrs-1); 
  for (int t=0; t<NumNbrs; t++) {
    x = perm[t]; assert((x>=0)&&(x<NumNbrs));
    nbr = lcn + Const().Bridge_offsets[x]; // nbr via bridge
    int c1 = lcn+Const().Nbr_offsets[x];     // carrier 
    int c2 = lcn+Const().Nbr_offsets[x+1];   // other carrier
    Move mv1(s,c1); 
    Move mv2(s,c2); 
    if (board[nbr] == s &&
        board[c1] == EMP &&
        board[c2] == EMP &&
        (not_in_miai(mv1)||not_in_miai(mv2))) {
          if (!not_in_miai(mv1)) {
              if (Const().near_edge(lcn) && Const().near_edge(c1)) 
              YborderRealign(Move(s,nbr),cpt,c1,reply[ndx(s)][c1],c2);
          }
          else if (!not_in_miai(mv2)) {
              if (Const().near_edge(lcn) && Const().near_edge(c2)) 
              YborderRealign(Move(s,nbr),cpt,c2,reply[ndx(s)][c2],c1);
         }
         else if (Find(parent,nbr)!=Find(parent,cpt)) {  // new miai
           nbrRoot = Find(parent,nbr);
           brdr[nbrRoot] |= brdr[cpt];
           cpt = Union(parent,cpt,nbrRoot); 
           set_miai(s,c1,c2);
           } 
         }
    else if ((board[nbr] == GRD) &&
             (board[c1]  == EMP) &&
             (board[c2]  == EMP) &&
             (not_in_miai(mv1))&&
             (not_in_miai(mv2))) { // new miai
      brdr[cpt] |= brdr[nbr];
      set_miai(s,c1,c2);
      }
  }
  bdset = brdr[cpt];
  return miReply;
}

int Board::move(Move mv, bool useMiai, int& bdset) 
{  //bdset comp. from scratch
    // put mv.s on board, update connectivity for mv.s
    // WARNING  oppnt(mv.s) connectivity will be broken if mv.s hits oppnt miai
    //   useMiai ? miai adjacency : stone adjacency
    // return opponent miai reply of mv, will be mv.lcn if no miai
    int nbr,nbrRoot,cpt; int lcn = mv.lcn; int s = mv.s;
    //if (brdr[lcn]!=BRDR_NIL) {
    //showAll();
    //prtLcn(lcn); printf("\n");
    //}
    //assert(brdr[lcn]==BRDR_NIL);
    
    m_toPlay = mv.s;
    put_stone(mv);
    FlipToPlay();
    
    cpt = lcn; // cpt of s group containing lcn
    for (int t=0; t<NumNbrs; t++) {
        nbr = lcn + Const().Nbr_offsets[t];
        if (board[nbr] == s) {
            nbrRoot = Find(parent,nbr);
            brdr[nbrRoot] |= brdr[cpt];
            cpt = Union(parent,cpt,nbrRoot); } 
        else if (board[nbr] == GRD) {
            brdr[cpt] |= brdr[nbr]; }
    }
    if (!useMiai) {
        bdset = brdr[cpt];
        return lcn;       // no miai, so return lcn
    } // else 
    int response = moveMiaiPart(mv, useMiai, bdset,cpt);
    
    if (has_win(bdset)) {
        m_winner = (mv.s == BLK) ? Y_BLACK_WINS : Y_WHITE_WINS;
    }
    return response;
}
