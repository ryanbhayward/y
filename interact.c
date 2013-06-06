#include <stdio.h>
#include "connect.h"
#include "genmove.h"
#include "interact.h"
#include "move.h"
#include "ui.h"

void interact(struct myboard* B) { int j;
  Bool_t quit = FALSE;
  Bool_t winner = FALSE;
  int stone = EMPTY_CELL;
  int opt_brd_set, brd_set = BORDER_NONE;
  int lcn = 0;
  char cmd = '!';  // initialize to unused character
  struct mymove hist [TotalCells];
  int moves_made = 0;
  Bool_t use_bridges = TRUE;
  Bool_t accelerate = TRUE;

  displayHelp();
  init_board(B);
  show_as_Rhombus(B->board);
  show_as_Rhombus(B->p);
  show_as_Rhombus(B->brdr);
  show_miai(B->reply[0]);
  show_miai(B->reply[1]);
  show_Yboard(B->board); 
  while(!quit) {
    getCommand(&stone,&lcn,&cmd);
    //printf("stone %d lcn %d cmd %c\n",stone,lcn,cmd);
    switch (cmd) {
      case QUIT_CH: quit = TRUE; break; 
      case UNDO_CH: 
        if (moves_made>0) { // start from scratch, replay all but last move
          winner = FALSE; // no moves are allowed after winner, so 
                          // no winner after undo
          init_board(B);
          // opponent moves can invalidate miais...
          // so put p1 stones, iteratively compute p2 connectivity,
          // leave p2 stones, remove p1 stones, iter. comp. p1 conn.
          int p1 = hist[0].stn;
          for (j=0; j<moves_made-1; j++) 
            if (p1==hist[j].stn)
              put_stone(p1,hist[j].lcn,B->board);
          for (j=0; j<moves_made-1; j++) 
            if (other_color(p1)==hist[j].stn) 
	      brd_set = make_move(hist[j].stn,hist[j].lcn,B,use_bridges);
          clear_stones(B,p1);
	  for (j=0; j<moves_made-1; j++) 
            if (p1==hist[j].stn)
	      brd_set = make_move(hist[j].stn,hist[j].lcn,B,use_bridges);
	  moves_made--;
        }
        show_Yboard(B->board); 
	break; 
      case GENMOVE_CH:
          if (winner) {
            printf("resign\n game is over\n");
            break;
          }
          lcn = monte_carlo(stone,B,accelerate);
          //lcn = rand_move(gbd);
	  printf("play %c %c%1d\n",
            emit(stone),alphaCol(lcn),numerRow(lcn)); //continue...
      default:
        if (winner) {
          printf("resign\n game is over\n");
          break;
        }
        if ((EMPTY_CELL != stone)&&(EMPTY_CELL != B->board[lcn]))
          printf("illegal (occupied or off-board)");
        else {
          brd_set = make_move(stone, lcn, B, use_bridges);
          hist[moves_made].stn = stone;
          hist[moves_made++].lcn = lcn;
          clear_stones(B,other_color(stone));
	  for (j=0; j<moves_made; j++) 
            if (other_color(stone)==hist[j].stn) {
	      opt_brd_set = make_move(hist[j].stn,hist[j].lcn,B,use_bridges);
            }
          show_Yboard(B->board); 
          winner = has_win(brd_set,stone);
          if (winner) {
             clear_stones(B,stone);
	     for (j=0; j<moves_made; j++) 
               if (stone==hist[j].stn) 
	         brd_set = make_move(hist[j].stn,hist[j].lcn,B,FALSE);
             winner = has_win(brd_set,stone);
             if (winner) {
                printf(" winner!!! ... "); emitString(stone);
                printf(" %c wins\n\n", emit(stone));
             }
             else 
               printf(" ... %c win detected ...\n",emit(stone)); 
           }
         }
    }
  }
  printf("\n\n ... thanks for the game ... :)\n\n"); 
}
