#include <cassert>
#include <cstdio>
#include "connect.h"
#include "genmove.h"
#include "interact.h"
#include "move.h"
#include "ui.h"

void printBye()             { printf("\n\n ... adios ... \n\n"); }
void printGameAlreadyOver() { printf("game over\n"); }
void printIllegal()         { printf("illegal... occupied or off-board"); }
void printHasWon(int s)     { printf(" %c wins\n\n", emit(s)); }
void printCanWin(int s)     { printf("  %c happy ...\n",emit(s)); }
void printMoveMade(Move m) {
  printf("play %c %c%1d\n", emit(m.s),alphaCol(m.lcn),numerRow(m.lcn)); }

int movePlus(Board& B, Move mv, bool miai, int&bdst, Move& h) { 
  // move, update history ... bdst computed from scratch
  h = mv;
  return B.move(mv,miai,bdst); }

bool check_hex_dimensions(int x, int y) {
  if ((x<1)||(y<1)) {
    printf("invalid hex board dimensions\n"); return false; }
  if (x+y>N+1) {
    printf("hex board dimensions too large\n"); return false; }
  printf("play %dx%d hex\n",x,y); 
  return true; }

void playHex(Board&B, Move h[], int& m, int x, int y) {
  if (check_hex_dimensions(x,y)) { int j,k,psn,dummy;
    for (j=0; j<N; j++)
      for (k=0; k<N-j; k++) { 
        psn = fatten(j,k);
        if (j>=x) 
          movePlus(B,Move(BLK,psn),false,dummy,h[m++]);
        else if (k>=y) 
            movePlus(B,Move(WHT,psn),false,dummy,h[m++]);
      }
  }
  //B.showAll(); }
  B.show(); }

int updateConnViaHistory(Board& B, int st, bool useMiai, Move h[], int mvs) { int bdst;
// needed eg if prev oppnt mv hits miai 
  B.zero_connectivity(st);
  for (int j=0; j<mvs; j++)
    if (st == h[j].s)
      B.move(h[j], useMiai, bdst);
  return bdst; }

void undoMove(Board& B, Move h[], int& moves, bool useMiai, bool& w) { 
  if (moves>0) { // start from scratch, replay all but last move
    w = false; // no moves after winner, so no winner after undo
    B.init(); int p1 = h[0].s;
    updateConnViaHistory(B, p1,        useMiai, h, moves-1);
    updateConnViaHistory(B, oppnt(p1), useMiai, h, moves-1);
    moves--;
  }
  B.show(); }

void moveAndUpdate(Board& B, Move mv, Move h[], int& m, bool useMiai, int& bdst, bool& w) {
  movePlus(B, mv, useMiai, bdst, h[m++]);
  updateConnViaHistory(B, oppnt(mv.s), useMiai, h, m);
  //B.showAll();
  B.show();
  w = has_win(bdst);
  if (w) { // 
    if (useMiai) { // check for absolute win
      bdst = updateConnViaHistory(B, mv.s, false, h, m);
      w = has_win(bdst);
    }
    if (w) // absolute win
      printHasWon(mv.s); 
    else        
      printCanWin(mv.s); 
  }
}

void interact(Board& B) { bool useMiai = true; bool accelerate = false;
  assert(B.num(EMP)==TotalCells);
  displayHelp(); 
  //B.showAll(); 
  B.show(); 
  Move h[TotalCells]; // history
  bool quit = false; bool winner = false; 
  int st, lcn, bdst;
  char cmd = UNUSED_CH;  // initialize to unused character
  int moves = 0;   // when parameter (eg miai-reply) not used
  while(!quit) {
    getCommand(cmd,st,lcn);
    switch (cmd) {
      case QUIT_CH:       quit = true;                  break; 
      case PLAYHEX_CH:  playHex (B, h, moves, st, lcn); break;  // st,lcn used for x,y
      case UNDO_CH:     undoMove(B, h, moves, useMiai, winner);  break;
      case GENMOVE_CH:
          if (winner)                            { printGameAlreadyOver(); break; }
          // lcn = monte_carlo(B, st, useMiai, accelerate);
          lcn = uct_move   (B, st, useMiai); 
          //lcn = rand_move  (B);
          printMoveMade( Move(st,lcn) );  // continue ...
      default: // specified move
        if (winner)                              { printGameAlreadyOver(); break; }
        if ((EMP != st)&&(EMP != B.board[lcn]))  { printIllegal(); break; }
        moveAndUpdate(B, Move(st,lcn), h, moves, useMiai, bdst, winner);
    }
  }
  printBye();
}
