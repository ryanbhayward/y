#include <stdio.h>
#include <stdlib.h>
#include "board.y.h"
#include "ui.h"

void displayHelp() {
  printf("\033[2J\033[1;1H"); // clear screen and reset cursor
  printf(" play Y     (RBH 2012, tips from Jakub)\n\n");  
  printf("   %c a3           (black a3)\n",BLACK_CH);
  printf("   %c %c       (black genmove)\n",BLACK_CH,GENMOVE_CH);
  printf("   %c c4           (white c4)\n",WHITE_CH);
  printf("   %c %c       (white genmove)\n",WHITE_CH,GENMOVE_CH);
  //printf("   %c d7     (erase d7)\n",EMPTY_CH);
  printf("   %c                  (undo)\n",UNDO_CH);
  printf("[return]              (quit)\n\n");
  //printf("         %c      (undo)\n",UNDO_CH); 
}

//int valid_index(int psn) {
  //return ((psn > 0)&&(psn <= N)) ? TRUE:FALSE;
//}

void getCommand(int *stone, int *lcn, char *cmd) {  int x,y;
  unsigned char ch; 
  Bool_t valid = FALSE; Bool_t quit = FALSE;
  *stone = EMPTY_CELL; *lcn = 0; 
  printf("\n=> ");
  while (!valid && !quit) {
    ch=getchar(); x=0; y=0;
    switch (ch) {
      case QUIT_CH:     ; // same as next case...
      case UNDO_CH:     *cmd = ch; quit  = TRUE;  break;
      case BLACK_CH:    *cmd = ch; *stone = BLACK_CELL;      break; 
      case WHITE_CH:    *cmd = ch; *stone = WHITE_CELL;      break; 
      default:   while (ch != '\n') ch = getchar();
    }
    if (!quit) {
      ch=getchar();  // skip this character, it should be blank
      ch=getchar();
    }
    if (ch == GENMOVE_CH) {
      *cmd = ch; quit = TRUE;
      //printf("genmove\n");
    }
    else { // parse the location
      x = 1+ch-'a';
      if (ch != '\n') ch=getchar();
      while ((ch >= '0') && (ch <= '9')) {
         y = 10*y+ch-'0';
         ch = getchar();
      }
    }
    while (ch != '\n') ch=getchar();
    if (quit || ((x>0)&&(x<=N)&&(y>0)&&(y<=N))) valid = TRUE;
    else {
      printf("x %d  y %d N %d \n",x,y,N);
      printf("invalid\n=> ");
    }
  }
  if (!quit) *lcn = (y+GUARDS-1)*(Np2G)+x+GUARDS-1; 
}
