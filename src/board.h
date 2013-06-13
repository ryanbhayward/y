#ifndef BOARDY_H
#define BOARDY_H

#include <string>
#include <vector>

#include "SgSystem.h"
#include "SgHash.h"
#include "SgBoardColor.h"
#include "move.h"
#include "VectorIterator.h"

static const int Y_INFINITY = 9999;

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


static const int Y_MAX_CELL = 255;
static const int Y_MAX_SIZE = 19;   // such that TotalCells < Y_MAX_CELL

static const int BRDR_NIL  = 0; // 000   border values, used bitwise
static const int BRDR_BOT   = 1; // 001
static const int BRDR_L     = 2; // 010
static const int BRDR_R     = 4; // 100
static const int BRDR_ALL   = 7; // 111

static const int TMP = 4;

static const int Y_SWAP = -1;
static const int Y_NULL_MOVE = -2;
 
static inline int oppnt(int x) {return 1-x; } // black,white are 0,1

static inline int ndx(int x) {return x;}  // assume black,white are 0,1

static const int NumNbrs = 6;              // num nbrs of each cell

struct Move {
  int s;
  int lcn;
  Move(int x, int y) : s(x), lcn(y) {}
  Move() {}
} ;

bool has_win(int bd_set) ;

class ConstBoard 
{
public:

    static const int GUARDS = 2; // must be at least one

    int m_size;
    int Np2G;
    int TotalCells;
    int TotalGBCells;

    int Nbr_offsets[NumNbrs+1];
    int Bridge_offsets[NumNbrs];

    ConstBoard(int size);

    static char ColorToChar(SgBoardColor color);
    static std::string ColorToString(SgBoardColor color);

    inline int fatten(int r,int c) const 
    {  return Np2G*(r+GUARDS) + (GUARDS+m_size-r-1+c); }

    inline int  board_row(int lcn) const 
    {  return (lcn/Np2G)-GUARDS;}

    inline int  board_col(int lcn) const 
    {
        int r = board_row(lcn);
        return (lcn%Np2G)-GUARDS-m_size+r+1;
    }

    inline bool near_edge(int lcn) const
    { 
        //static inline bool near_edge(int lcn,int d) { 
        // near_edge(lcn,0) true if lcn on edge
        // near_edge(lcn,1) true if lcn 1-away from edge
        int r = board_row(lcn);
        int c = board_col(lcn);
        //return ((d==r)||(d==c)||(N==r+c+d+1));
        return ((0==r)||(0==c)||(m_size==r+c+1));
    }

    int Size() const { return m_size; }

    bool IsOnBoard(int cell) const
    {
        return std::find(m_cells.begin(), m_cells.end(), cell) != m_cells.end();
    }

    std::string ToString(int cell) const;
    int FromString(const std::string& name) const;

private:
    std::vector<int> m_cells;

    friend class BoardIterator;

    friend class Board;
};

struct Board 
{
    ConstBoard m_constBrd;

    std::vector<SgBoardColor> board;
    std::vector<int> parent;   
    std::vector<int> brdr;
    std::vector<int> reply[2];  // miai reply
    
    SgBlackWhite m_toPlay;
    SgBoardColor m_winner;  

    Board();
    Board(int size); // constructor

    const ConstBoard& Const() const { return m_constBrd; }

    int Size() const { return Const().Size(); }

    std::string ToString() const;
    std::string BorderToString() const;

    SgHashCode Hash() const { return 1; /* FIXME: IMPLEMENT HASH! */ };

    bool IsGameOver() const { return m_winner != SG_EMPTY; }
    SgBoardColor GetWinner() const { return m_winner; }
    bool IsWinner(SgBlackWhite player) const { return player == m_winner; }

    bool CanSwap() const { return false; }
    void Swap();

    bool IsOccupied(int cell) const { return board[cell] != SG_EMPTY; }
    bool IsEmpty(int cell) const { return board[cell] == SG_EMPTY; };

    int ToPlay() const         { return m_toPlay; }
    void SetToPlay(int toPlay) { m_toPlay = toPlay; }

    void zero_connectivity(int s, bool removeStone = true);
    void set_miai    (int s, int m1, int m2);
    void release_miai(Move mv);
    bool not_in_miai (Move mv);
    void put_stone   (Move mv);
    void remove_stone(int lcn);
    int  move(Move mv, bool useMiai, int& brdset); // ret: miaiMove
    int  moveMiaiPart(Move mv, bool useMiai, int& brdset, int cpt);
    void YborderRealign(Move mv, int& cpt, int c1, int c2, int c3);
    void show();

private:
  
    void init();

    void FlipToPlay()
    {
        m_toPlay = (m_toPlay == SG_BLACK) ? SG_WHITE : SG_BLACK;
    }

};

//----------------------------------------------------------------------

class BoardIterator : public VectorIterator<int>
{
public:
    BoardIterator(const Board& brd);

    BoardIterator(const ConstBoard& brd);
};

inline BoardIterator::BoardIterator(const Board& brd)
    : VectorIterator<int>(brd.Const().m_cells)
{
}

inline BoardIterator::BoardIterator(const ConstBoard& brd)
    : VectorIterator<int>(brd.m_cells)
{
}

//----------------------------------------------------------------------

extern const int Nbr_offsets[NumNbrs+1] ;  // last = 1st to avoid using %mod
extern const int Bridge_offsets[NumNbrs] ;

static const int RHOMBUS = 0;
static const int TRI   = 1;

void shapeAs(int shape, int X[]);
void showYcore(int X[]) ;  // simple version of Board.show
void display_nearedges();

#endif
