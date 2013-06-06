#include <stdio.h>
#include <stdlib.h>
#include "board.y.h"

const int Nbr_origin_offsets[NumNbrsp1]   = {-Np2G, 1-Np2G,1,Np2G,Np2G-1,-1,-Np2G};
const int Bridge_origin_offsets[NumNbrs] = {-2*Np2G+1,-Np2Gm2,Np2Gp1,2*Np2G-1,Np2Gm2,-Np2Gp1};
//const int Nbr_loop_offsets[NumNbrs]      = { -Np2G, 1, Np2Gm2, 2, Np2Gm2, 1 };

char emit(int value) {
  switch(value) {
    case EMPTY_CELL: return EMPTY_CH;
    case BLACK_CELL: return BLACK_CH;
    case WHITE_CELL: return WHITE_CH;
    default: return '?';
  }
}

void emitString(int value) {
  switch(value) {
    case EMPTY_CELL: printf("empty"); break;
    case BLACK_CELL: printf("black"); break;
    case WHITE_CELL: printf("white"); break;
    default: printf(" ? ");
  }
}

// display vector as Np2G*Np2G rhombus
void show_as_Rhombus(int X[]) { int j,k;
  for (j = 0; j < Np2G; j++) {
    for (k = 0; k < j; k++) 
      printf(" ");
    for (k = 0; k < Np2G; k++) {
      printf(" %3d", X[k+j*Np2G]); 
    }
    printf("\n");
  }
  printf("\n");
}

void show_as_Y(int X[]) { int j,k,psn;
  psn = 0;
  for (j = 0; j < N; j++) {
    for (k = 0; k < j; k++) 
      printf(" ");
    for (k = 0; k < N-j; k++) {
      printf(" %3d", X[psn]);
      psn++;
    }
    printf("\n");
  }
  printf("\n");
}

void show_inner_Y(int X[]) { int j,k,psn,x;
  for (j = 0; j < N; j++) {
    psn = (j+GUARDS)*Np2G + GUARDS; //printf("psn %2d ",psn);
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
  printf("\n");
}

void show_miai(int X[]) { int j,k,psn,x;
  for (j = 0; j < N; j++) {
    psn = (j+GUARDS)*Np2G + GUARDS; //printf("psn %2d ",psn);
    for (k = 0; k < j; k++) 
      printf(" ");
    for (k = 0; k < N-j; k++) {
      x = X[psn++];
      if (x!=psn-1)
        printf(" %3d", x);
      else
        printf("   *");
    }
    printf("\n");
  }
  printf("\n");
}

void show_Yboard(int X[]) { int j,k; char ch;
  printf("\n   ");
  for (ch='a'; ch < 'a'+N; ch++) printf(" %c ",ch); printf("\n\n");
  for (j = 0; j < N; j++) {
    for (k = 0; k < j; k++) 
      printf(" ");
    printf("%2d   ",j+1);
    for (k = 0; k < N-j; k++) 
      printf("%c  ", emit(X[(j+GUARDS)*Np2G+k+GUARDS]));
    printf("\n");
  }
  printf("\n");
}

int numerRow(int psn) { 
  return psn/Np2G - (GUARDS-1);
}

char alphaCol(int psn) { 
  return 'a' + psn%Np2G-GUARDS;
}

void clear_stones(struct myboard* myB, int stone) { int j;
  for (j=0; j<TotalGBCells; j++) {
    myB->reply[ndx(stone)][j] = j;
    if (myB->board[j]==stone) {
      myB->board[j]= EMPTY_CELL;
      myB->p[j]    = j;
      myB->brdr[j] = BORDER_NONE;
    }
  }
  printf("clear_stones %c\n",emit(stone));
  show_Yboard(myB->board);
  show_miai(myB->reply[ndx(stone)]);
  show_miai(myB->reply[ndx(other_color(stone))]);
  printf("leaving clear_stones %c\n",emit(stone));
}

void init_board(struct myboard* myB ) { int j,k;
  for (j=0; j<TotalGBCells; j++) {
    myB->board[j]       = GUARD_CELL;
    myB->brdr[j] = BORDER_NONE;
    myB->p[j]  = j;
    for (k=0; k<2; k++) 
      myB->reply[k][j] = j;
  }
  for (j=0; j<N; j++) 
    for (k=0; k<N-j; k++)
      myB->board[fatten(j,k)] = EMPTY_CELL;
  for (j=0; j<Np1; j++) 
    myB->brdr[fatten(0,0)+j-Np2G] = BORDER_TOP;
  for (j=0; j<Np1; j++) 
    myB->brdr[fatten(0,0)+j*Np2G-1] = BORDER_LEFT;
  for (j=0; j<Np1; j++) 
    myB->brdr[fatten(0,0)+j*(Np2G-1)+N] = BORDER_RIGHT;
}
