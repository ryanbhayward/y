#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

#include "board.h"
#include "move.h"
#include "connect.h"

using namespace std;

char ConstBoard::ColorToChar(SgBoardColor color) 
{
    switch(color) {
    case SG_BLACK: return 'b';
    case SG_WHITE: return 'w';
    case SG_EMPTY: return '.';
    default: return '?';
    }
}

std::string ConstBoard::ColorToString(SgBoardColor color) 
{
    switch(color) {
    case SG_BLACK: return "black";
    case SG_WHITE: return "white";
    case SG_EMPTY: return "empty";
    default: return "?";
    }
}

std::string ConstBoard::ToString(int cell) const
{
    char str[16];
    sprintf(str, "%c%1d",board_col(cell)+'a',board_row(cell)+1); 
    return str;
}

int ConstBoard::FromString(const string& name) const
{
    if (name == "swap")
    	return Y_SWAP;
    int x = name[0] - 'a' + 1;
    std::istringstream sin(name.substr(1));
    int y;
    sin >> y;
    return fatten(y-1, x-1);
}

ConstBoard::ConstBoard(int size)
    : m_size(size)
    , Np2G(m_size+2*GUARDS)
    , TotalCells(m_size*(m_size+1)/2)
    , TotalGBCells(Np2G*Np2G)
{
    Nbr_offsets[0] =  -Np2G;
    Nbr_offsets[1] = 1-Np2G;
    Nbr_offsets[2] = 1;
    Nbr_offsets[3] = Np2G;
    Nbr_offsets[4] = Np2G-1;
    Nbr_offsets[5] = -1;
    Nbr_offsets[6] = -Np2G;

    Bridge_offsets[0] = -2*Np2G+1;
    Bridge_offsets[1] = 2-Np2G;
    Bridge_offsets[2] = Np2G+1;
    Bridge_offsets[3] = 2*Np2G-1;
    Bridge_offsets[4] = Np2G-2;
    Bridge_offsets[5] = -(Np2G+1);
    m_cells.clear();

    for (int r=0; r<Size(); r++)
        for (int c=0; c<=r; c++)
            m_cells.push_back(fatten(r,c));
}

#if 0
///////////// Playout::

void Playout::single_playout(int& turn, int& k, bool useMiai) { 
  Board L(B);
  k = -1; // k+1 is num stones placed before winner found
  int bd_set = BRDR_NIL; 
  shuffle_interval(Avail, 0, numAvail-1);
  do { 
    k++;
    if (L.board[Avail[k]]!=EMP) {
      printf("k %d  turn %d\n",k,turn);
      L.show();
    }
    assert(L.board[Avail[k]]==EMP);

    Move mv(turn, Avail[k]);
    int miReply = L.move(mv, useMiai, bd_set);
    // played into oppt miai ?
    int resp = L.reply[ndx(oppnt(turn))][Avail[k]];
    if (resp!=Avail[k]) {//prep for autorespond on next move
      int z;
      for (z=k+1; Avail[z]!=resp; z++) ;
      if (z>=TotalCells) {
       printf("miai problem   resp %d k %d turn %d\n",resp,k,turn);
       B.show(); L.show(); shapeAs(RHOMBUS,Avail);
       L.showMi(oppnt(turn)); B.showMi(oppnt(turn));
       L.showMi(turn); B.showMi(turn);
      }
      assert(z<TotalCells); assert((k+1)<TotalCells);
      swap(Avail[k+1],Avail[z]);
    }
    turn = oppnt(turn);
  } while (!has_win(bd_set)) ; }

Playout::Playout(Board& B):B(B) 
{ 
    int psn; int j=0;
    for (BoardIterator it(B); it; ++it) {
        if (EMP==B.board[*it])
            Avail[j++]=psn;
    }
    assert(j!=0); // must be available move
    numAvail = j;
    numWinners = 0;
    memset(wins,0,sizeof(wins));
    memset(winsBW,0,sizeof(winsBW));
    memset(colorScore,0,sizeof(colorScore));
    memset(win_length,0,sizeof(win_length)); 
}
#endif

//////////////////// Board::
 
#if 0
void Board::showMi(int s) 
{ 
    for (int j = 0; j < N; j++) {
        int psn = (j+GUARDS)*Np2G + GUARDS; //printf("psn %2d ",psn);
        for (int k = 0; k < j; k++) 
            printf(" ");
        for (int k = 0; k < N-j; k++) {
            int x = reply[ndx(s)][psn++];
            if (x!=psn-1)
                printf(" %3d", x);
            else
                printf("   *");
        }
        printf("\n");
    }
    printf("\n"); 
}

void Board::showP() 
{
    printf("UF parents (for non-captains only)\n");
    for (int j = 0; j < N; j++) {
        int psn = (j+GUARDS)*Np2G + GUARDS; //printf("psn %2d ",psn);
        for (int k = 0; k < j; k++) 
            printf(" ");
        for (int k = 0; k < N-j; k++) {
            int x = parent[psn++];
            if (x!=psn-1)
                prtLcn(x);//printf(" %3d", x);
            else
                printf("   *");
        }
        printf("\n");
    }
    printf("\n"); }

void Board::showBr() 
{ 
    printf("border values\n");
    int psn = 0;
    for (int j = 0; j < Np2G; j++) {
        for (int k = 0; k < j; k++)
            printf(" ");
        for (int k = 0; k < Np2G; k++) {
            int x = brdr[psn++];
            if (x != BRDR_NIL)
                printf(" %3d", x);
            else
                printf("  * ");
        }
        printf("\n");
    }
    printf("\n"); 
}

#endif

std::string Board::ToString() const
{
    ostringstream os;
    const int N = Size();
    os << "\n";
    for (int j = 0; j < N; j++) {
        for (int k = 0; k < N-j; k++) 
            os << ' ';
        os << (j+1<10?" ":"") << j+1 << "  "; 
        for (int k = 0; k <= j ; k++) 
            os << ConstBoard::ColorToChar(board[Const().fatten(j,k)]) << "  ";
        os << "\n";
    }
    os << "   ";
    for (char ch='a'; ch < 'a'+N; ch++) 
        os << ' ' << ch << ' '; 

#if 0
    const int Np2G = Const().Np2G;
    printf("border values\n");
    int psn = 0;
    for (int j = 0; j < Np2G; j++) {
        for (int k = 0; k < j; k++)
            printf(" ");
        for (int k = 0; k < Np2G; k++) {
            int x = brdr[ConstFind(parent,psn++)];
            if (x != BRDR_NIL)
                printf(" %2d", x);
            else
                printf("  *");
        }
        printf("\n");
    }
    printf("\n"); 
#endif

    return os.str();
}

std::string Board::BorderToString() const
{
  ostringstream os;
  const int Np2G = Const().Np2G;
  os << "\n" << "Border Values:\n";
  int psn = 0;
  for (int j = 0; j < Np2G; j++) {
    for (int k = 0; k < j; k++)
      os << ' ';
    for (int k = 0; k < Np2G; k++) {
      int x = brdr[ConstFind(parent,psn++)];
      if (x != BRDR_NIL)
        os << "  " << x;
      else
        os << "  *";
    }
    os << "\n";
  }
  os << "\n"; 
  return os.str();
}

void Board::show() 
{
    printf("%s\n", ToString().c_str());
}

void Board::zero_connectivity(int stone, bool remS) 
{ 
    for (int j=0; j<Const().TotalGBCells; j++) {
        reply[ndx(stone)][j] = j;
        if (board[j]==stone) {
            parent[j] = j;
            brdr[j]   = BRDR_NIL;
            board[j]  = TMP;
            if (remS)
                board[j]= SG_EMPTY;
        }
    } 
}

void Board::init() 
{ 
    const int N = Size();
    const int T = Const().TotalGBCells;

    board.resize(T);
    parent.resize(T);
    brdr.resize(T);
    reply[0].resize(T);
    reply[1].resize(T);

    for (int j=0; j<T; j++) {
        board[j] = SG_BORDER; 
        brdr[j]  = BRDR_NIL; 
        parent[j]  = j;
        for (int k=0; k<2; k++) 
            reply[k][j] = j;
    }
    for (BoardIterator it(Const()); it; ++it)
        board[*it] = SG_EMPTY;

    for (int j=0; j<N; j++) { 
        brdr[Const().fatten(j,0)-1] = BRDR_L;
        brdr[Const().fatten(j,j)+1]   = BRDR_R;
        brdr[Const().fatten(N,j)]   = BRDR_BOT;
    }
    brdr[Const().fatten(-1,0)-1] = BRDR_L;
    brdr[Const().fatten(-1,-1)+1] = BRDR_R;
    brdr[Const().fatten(N,N)] = BRDR_BOT;
 
    m_winner = Y_NO_WINNER;
#if 0
    const int Np2G = Const().Np2G;
    printf("border values\n");
    int psn = 0;
    for (int j = 0; j < Np2G; j++) {
        for (int k = 0; k < j; k++)
            printf(" ");
        for (int k = 0; k < Np2G; k++) {
            int x = brdr[psn++];
            if (x != BRDR_NIL)
                printf(" %2d", x);
            else
                printf("  *");
        }
        printf("\n");
    }
    printf("\n"); 
#endif
}

Board::Board()
    : m_constBrd(10)
{
    init();
}

Board::Board(int size) 
    : m_constBrd(size)
{ 
    init(); 
}

void Board::Swap()
{
    // swap black stones with white stones
}

//////////////////////////////////////////////////////////////////////
