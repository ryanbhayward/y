#ifndef BOARDY_H
#define BOARDY_H

#define INFINITY 9999
#define Bool_t int
#define FALSE 0
#define TRUE !FALSE

//  Y-board 
//          N  cells per side        N(N+1)/2  total cells
//  fat board, a.k.a. guarded board with GUARDS extra rows/cols, allows
//     fixed offsets for nbrs at dist 1 or 2, e.g. below with GUARDS = 2
//
//          . . g g g g g g g 
//           . g g g g g g g g 
//            g g * * * * * g g 
//             g g * * * * g g . 
//              g g * * * g g . . 
//               g g * * g g . . . 
//                g g * g g . . . . 
//                 g g g g . . . . . 
//                  g g g . . . . . . 
#define N 8
#define NumNbrs 6  // number of neighbours of each cell
#define NumNbrsp1 (NumNbrs+1)
#define GUARDS 1  // must be at least one
#define Np1 (N+1)
#define Np2 (N+2)
#define Np2G (N+2*GUARDS)
#define Np2Gm2 (Np2G-2)
#define Np2Gp1 (Np2G+1)
#define NNp1 (N*Np1)
#define TotalCells ((NNp1)/2)
#define TotalGBCells (Np2G*Np2G)
#define B0 (2*Np2G-1) // bridge offsets
#define B1 (Np2Gm2)
#define B2 (Np2Gp1)

static const int BORDER_NONE  = 0;
static const int BORDER_TOP   = 1;
static const int BORDER_LEFT  = 2;
static const int BORDER_RIGHT = 4;
static const int BORDER_ALL   = 7;

#define EMPTY_CELL 0  
#define BLACK_CELL 1  
#define WHITE_CELL 2
#define GUARD_CELL 3   
#define other_color(x) (3-x)  // assume black,white are 1,2
#define ndx(x)    (x-1)  // assume black,white are 1,2

#define BLACK_CH 'b'
#define WHITE_CH 'w'
#define EMPTY_CH '.'

#define fatten(r,c) (Np2G*(r+GUARDS)+(c+GUARDS))

struct mymove {
  int stn;
  int lcn;
} ;
struct myboard {
  int board[TotalGBCells];
  int p    [TotalGBCells];
  int brdr [TotalGBCells];
  //int num_miai[2];             // number of committed miai
  int reply[2][TotalGBCells];  // miai reply
} ;

extern const int Nbr_origin_offsets[NumNbrsp1] ;  // last = 1st, avoid mod
extern const int Bridge_origin_offsets[NumNbrs] ;
extern void show_as_Rhombus(int X[]) ;
extern void show_as_Y(int X[]) ;
extern void show_inner_Y(int X[]) ;
extern void show_miai(int X[]) ;
extern void show_Yboard(int X[]) ;
extern char emit(int value) ;
extern void emitString(int value) ;
extern int numerRow(int psn) ;
extern char alphaCol(int ) ;
extern void init_board(struct myboard* myB ) ;
extern void clear_stones(struct myboard* myB, int stone) ;
#endif
