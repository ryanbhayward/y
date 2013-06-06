#include "node.h"
#include "move.h"
#include <cmath>
#include <cassert>
#include <cstdio>

int Node::uct_playout(Board& B, int plyr, bool useMiai) { 
  if (isLeaf()) 
    if (stat.n >= EXPAND_THRESHOLD) 
       expand(B,plyr);
  int winner;
  if (isLeaf()) {
    Playout pl(B);
    winner = plyr; 
    int moves_to_win;
    pl.single_playout(winner, moves_to_win, useMiai);
    winner = oppnt(winner);
  }
  else {
    Child& c  = children[bestChildNdx()];
    int bdset = BRDR_NIL;
    int miReply = B.move(Move(plyr, c.lcn), useMiai, bdset);
    if (miReply!=c.lcn) {
      //printf("%c",emit(plyr));prtLcn(c.lcn);
      //printf("has miReply");prtLcn(miReply);printf("\n");
      B.zero_connectivity(oppnt(plyr), false);  // ... but do not remove stones
      //B.showBr();
      // now use stones to update connectivity
      int opponent_bdst;
      for (int j=0; j<TotalGBCells; j++) 
        if (B.board[j]==TMP) {
          //prtLcn(j); printf(" conn update plyr %c\n",emit(oppnt(plyr)));
	  // need to update opponent's board, but opponent_bdst will not be used
	  // TODO: move the recomputation of board connectivity into board
          B.move(Move(oppnt(plyr),j), useMiai, opponent_bdst);
        }
    }
    if (has_win(bdset)) { // save recursion, update child stats
      winner = plyr;
      this->proofStatus = PROVEN_WIN;
      c.node.proofStatus = PROVEN_LOSS;
      c.node.stat.n++;
      }
    else
      winner = c.node.uct_playout(B, oppnt(plyr), useMiai);
  }
  // update stats
  stat.n++;
  if (winner==plyr) stat.w++;
  return winner;
}

Node::~Node() {  // destructor
  if (children != NULL) {
    delete[] children; children = NULL;
  }
}

void Node::expand(Board& B, int s) { 
  assert(isLeaf());
  int c = numChildren = B.num(EMP);
  assert(c>0);
  children = new Child[c];
  c = 0;
  for (int j=0; j<TotalGBCells; j++)
    if (B.board[j]==EMP) 
      children[c++].lcn =j;
}

int Node::bestMove() {
  assert (!isLeaf());
  int score = -1; // assume less than any ucb score
  int lcn = 0;
  for (int j=0; j<numChildren; j++) {
    int score0 = (children[j].node.proofStatus == PROVEN_LOSS) ? INFINITY : children[j].node.stat.n;
    if (score0 > score) {
      score = score0;
      lcn = children[j].lcn;
    }
  }
  return lcn;
}

int Node::bestChildNdx() {
  assert (!isLeaf());
  float score = -1; // assume less than any ucb score
  int ndx = 0;
  for (int j=0; j<numChildren; j++) {
    float score0 = ucb_eval(children[j].node);
    if (score0 > score) {
      score = score0;
      ndx = j;
    }
  }
  return ndx;
}

float Node::ucb_eval(Node& child) { 
  // child.stat.w/child.s.n gives prob of opponent win at child state
  Stats cs = child.stat;
  if (cs.n==0) return INFINITY;
  return (1.0 - float(cs.w)/cs.n) + UCB_EXPLORE*sqrt(sqrt(stat.n)/cs.n);
}
