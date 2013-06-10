#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

#include "board.h"
#include "move.h"
#include "shuff.h"

using namespace std;

void prtLcn(int psn) {
  printf(" %c%1d",alphaCol(psn),numerRow(psn)); 
}

char ColorToChar(int value) 
{
  switch(value) {
    case EMP: return EMP_CH;
    case BLK: return BLK_CH;
    case WHT: return WHT_CH;
    default: return '?'; 
  }
}
void ColorToString(int value) 
{
  switch(value) {
    case EMP: printf("empty"); break;
    case BLK: printf("black"); break;
    case WHT: printf("white"); break;
    default: printf(" ? "); } 
}

int b2(int shape, int j) 
{ // inner loop bound in shapeAs
    switch(shape) {
    case RHOMBUS: return Np2G;  // will display as Np2G*Np2G rhombus
    case TRI:     return N-j;   // will display as N*N*N triangle
    default:      assert(0==1); return 0; } 
}

void shapeAs(int shape, int X[]) { // display as given shape
  int psn = 0; int b = (shape==RHOMBUS)? Np2G : N;
  for (int j = 0; j < b; j++) {
    for (int k = 0; k < j; k++) printf(" ");
    for (int k = 0; k < b2(shape,j); k++) 
      printf(" %3d", X[psn++]); 
    printf("\n"); }
  printf("\n"); }

void showYcore(int X[]) { int j,k,psn,x;
  for (j = 0; j < N; j++) {
    psn = fatten(j,0); 
    for (k = 0; k < j; k++) 
      printf(" ");
    for (k = 0; k < N-j; k++) {
      x = X[psn++];
      if (x!=0)
        printf(" %3d", x);
      else
        printf("   *");
    }
    printf("\n");
  }
  printf("\n"); }

void display_nearedges() { int T[TotalGBCells];
 for (int j=0; j<TotalGBCells; j++)
    T[j]=0;
  for (int j=0; j<TotalGBCells; j++)
    if ((board_row(j)>=0)&&(board_col(j)>=0)&&
        (board_row(j)<N)&&(board_col(j)<N)&&
        (board_row(j)+board_col(j)<N)&&near_edge(j))
      T[j]=9;
  shapeAs(RHOMBUS,T); }

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
    int miReply = -1;
    Move mv(turn, Avail[k]);
    miReply = L.move(mv, useMiai, bd_set);
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

Playout::Playout(Board& B):B(B) { int psn; int j=0;
// put locations of available moves in Avail, return how many
  for (int r=0; r<N; r++)
    for (int c=0; c<N-r; c++) {
      psn = fatten(r,c);
      if (EMP==B.board[psn])
        Avail[j++]=psn;
      }
  assert(j!=0); // must be available move
  numAvail = j;
  numWinners = 0;
  memset(wins,0,sizeof(wins));
  memset(winsBW,0,sizeof(winsBW));
  memset(colorScore,0,sizeof(colorScore));
  memset(win_length,0,sizeof(win_length)); }

//////////////////// Board::

int Board::num(int kind) { int count = 0;
  for (int j=0; j<N; j++)
    for (int k=0; k<N; k++)
        if (board[fatten(j,k)]==kind) 
          count ++;
  return count; }
  
void Board::showMi(int s) { ColorToString(s), printf(" miai\n");
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
  printf("\n"); }

void Board::showP() { printf("UF parents (for non-captains only)\n");
  for (int j = 0; j < N; j++) {
    int psn = (j+GUARDS)*Np2G + GUARDS; //printf("psn %2d ",psn);
    for (int k = 0; k < j; k++) 
      printf(" ");
    for (int k = 0; k < N-j; k++) {
      int x = p[psn++];
      if (x!=psn-1)
        prtLcn(x);//printf(" %3d", x);
      else
        printf("   *");
    }
    printf("\n");
  }
  printf("\n"); }

void Board::showBr() { printf("border values\n");
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
  printf("\n"); }

std::string Board::ToString(int cell) const
{
    char str[16];
    sprintf(str, "%c%1d",alphaCol(cell),numerRow(cell)); 
    return str;
}

std::string Board::ToString() const
{
    ostringstream os;
    os << "\n   ";
    for (char ch='a'; ch < 'a'+m_size; ch++) 
        os << ' ' << ch << ' '; 
    os << "\n\n";
    for (int j = 0; j < m_size; j++) {
        for (int k = 0; k < j; k++) 
            os << ' ';
        os << j+1 << "   ";
        for (int k = 0; k < m_size-j; k++) 
            os << ColorToChar(board[fatten(j,k)]) << "  ";
        os << '\n';
    }
    return os.str();
}

void Board::show() 
{
    printf("%s\n", ToString().c_str());
}

void Board::showAll() {
  showMi(BLK);
  showMi(WHT);
  showP();
  showBr();
  show();
}

void Board::zero_connectivity(int stone, bool remS) { //printf("Z ");
  for (int j=0; j<TotalGBCells; j++) {
    reply[ndx(stone)][j] = j;
    if (board[j]==stone) {
      //prtLcn(j);
      p[j]    = j;
      brdr[j] = BRDR_NIL;
      board[j] = TMP;
      if (remS)
        board[j]= EMP;
    }
  } 
  //printf("\n");
}

void Board::init() { int j,k;
  for (j=0; j<TotalGBCells; j++) {
    board[j] = GRD; brdr[j]  = BRDR_NIL; p[j]  = j;
    for (k=0; k<2; k++) 
      reply[k][j] = j;
  }
  for (j=0; j<N; j++) 
    for (k=0; k<N-j; k++)
      board[fatten(j,k)] = EMP;
  for (j=0; j<N+1; j++) 
    brdr[fatten(0,0)+j-Np2G] = BRDR_TOP;
  for (j=0; j<N+1; j++) 
    brdr[fatten(0,0)+j*Np2G-1] = BRDR_L;
  for (j=0; j<N+1; j++) 
    brdr[fatten(0,0)+j*(Np2G-1)+N] = BRDR_R;
}

Board::Board()
    : m_size(10)
{
    init();
}

Board::Board(int size) 
    : m_size(size)
{ 
    init(); 
}

int Board::FromString(const string& name) const
{
    if (name == "swap")
    	return Y_SWAP;
    int x = name[0] - 'a' + 1;
    std::istringstream sin(name.substr(1));
    int y;
    sin >> y;
    return board_format(x, y);
}

bool Board::CanSwap() const { return false; }

void Board::Swap()
{
    // swap black stones with white stones
}

bool Board::IsOccupied(int cell) const
{
    return board[cell] != EMP;
}

bool Board::IsWinner(int player) const
{
    if (m_winner != Y_NO_WINNER) {
        if (player == BLK)
            return m_winner == Y_BLACK_WINS;
        if (player == WHT)
            return m_winner == Y_WHITE_WINS;
    }
    return false;
}

void Board::FlipToPlay()
{
    m_toPlay = (m_toPlay == BLK) ? WHT : BLK;
}
