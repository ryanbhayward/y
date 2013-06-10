#ifndef BOARDY_H
#define BOARDY_H
#include "move.h"

static const int INFINITY = 9999;

//  Y-board       N cells per side    N(N+1)/2  total cells
//  fat board, aka. guarded board with GUARDS extra rows/cols,
//  allows fixed offsets for nbrs @ dist 1 or 2, 
//  e.g. below with GUARDS = 2
//          . . g g g g g g g 
//           . g g g g g g g g 
//            g g * * * * * g g 
//             g g * * * * g g . 
//              g g * * * g g . . 
//               g g * * g g . . . 
//                g g * g g . . . . 
//                 g g g g . . . . . 
//                  g g g . . . . . . 

static const int N = 10;
static const int GUARDS = 2; // must be at least one
static const int Np2G = N+2*GUARDS;
static const int TotalCells = N*(N+1)/2;
static const int TotalGBCells = Np2G*Np2G;

static const int BRDR_NIL  = 0; // 000   border values, used bitwise
static const int BRDR_TOP   = 1; // 001
static const int BRDR_L     = 2; // 010
static const int BRDR_R     = 4; // 100
static const int BRDR_ALL   = 7; // 111

static const int EMP = 0; // empty cell
static const int BLK = 1; // black "
static const int WHT = 2; // white "
static const int GRD = 3; // guard "
static const int TMP = 4;

static const char BLK_CH = 'b';
static const char WHT_CH = 'w';
static const char EMP_CH = '.';

static inline int oppnt(int x) {return 3^x;} // black,white are 1,2
static inline int ndx(int x) {return x-1;}  // assume black,white are 1,2

char emit(int value) ;       // EMP, BLK, WHT
void emitString(int value) ; // empty, black, white
void prtLcn(int psn);        // a5, b13, ...
static inline int fatten(int r,int c) {return Np2G*(r+GUARDS)+(c+GUARDS);}
static inline int board_row(int lcn) {return (lcn/Np2G)-GUARDS;}
static inline int board_col(int lcn) {return (lcn%Np2G)-GUARDS;}
static inline int  numerRow(int psn) {return psn/Np2G - (GUARDS-1);}
static inline char alphaCol(int psn) {return 'a' + psn%Np2G - GUARDS;}
static inline bool near_edge(int lcn) { 
//static inline bool near_edge(int lcn,int d) { 
// near_edge(lcn,0) true if lcn on edge
// near_edge(lcn,1) true if lcn 1-away from edge
  int r = board_row(lcn);
  int c = board_col(lcn);
  //return ((d==r)||(d==c)||(N==r+c+d+1));
  return ((0==r)||(0==c)||(N==r+c+1));
}


struct Board {
  int board[TotalGBCells];
  int p    [TotalGBCells];
  int brdr [TotalGBCells];
  int reply[2][TotalGBCells];  // miai reply
  void init();
  void zero_connectivity(int s, bool removeStone = true);
  void set_miai    (int s, int m1, int m2);
  void release_miai(Move mv);
  bool not_in_miai (Move mv);
  void put_stone   (Move mv);
  void remove_stone(int lcn);
  int  move(Move mv, bool useMiai, int& brdset); // ret: miaiMove
  int  moveMiaiPart(Move mv, bool useMiai, int& brdset, int cpt); // miai part of move
  void YborderRealign(Move mv, int& cpt, int c1, int c2, int c3);
  int num(int cellKind);
  void show();
  void showMi(int s);
  void showP();
  void showBr(); //showBorder connectivity values
  void showAll();
  Board(); // constructor
} ;

struct Playout {
  int Avail [TotalCells]; // locations of available cells
  int numAvail;           // number of available cells
  int numWinners;         // number diff't cells that have won playouts
  int wins[TotalGBCells];
  int winsBW[2][TotalGBCells];
  int colorScore[2]; // black, white
  int win_length[2]; // sum (over wins) of stones in win
  Playout(Board& B); // constructor
  void single_playout(int& turn, int& moves_to_winner, bool useMiai); //
  Board& B;
} ;

extern const int Nbr_offsets[NumNbrs+1] ;  // last = 1st to avoid using %mod
extern const int Bridge_offsets[NumNbrs] ;

static const int RHOMBUS = 0;
static const int TRI   = 1;
void shapeAs(int shape, int X[]);
void showYcore(int X[]) ;  // simple version of Board.show
void display_nearedges();
#endif
