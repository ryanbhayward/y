#ifndef NODE_H
#define NODE_H
#include "board.h"
#include <cstddef>

static const int UNKNOWN    = 2;
static const int PROVEN_WIN  = 1;
static const int PROVEN_LOSS = 0;

static const float UCB_EXPLORE = 1.0;
static const int EXPAND_THRESHOLD = 2;

struct Stats {
  int w; // number of wins for player to move from this state
  int n; // number of playouts                from this state
  Stats() : w(0), n(0) { }
} ;

struct Child ; // forward ref

struct Node {
  Stats stat;
  int proofStatus;
  Child* children;
  int numChildren;

  Node() : proofStatus(UNKNOWN), children(NULL), numChildren(0) { } // constructor
 ~Node() ; // destructor

  void expand (Board&, int s) ;
  int bestChildNdx() ;
  int bestMove() ;
  bool isLeaf() {return children==NULL; } 
  float ucb_eval(Node& child) ;
  int uct_playout(Board&, int s, bool useMiai) ;
} ;

struct Child {
  Node node;
  int lcn;
} ;
#endif
