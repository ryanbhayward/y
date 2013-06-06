#include <stdio.h>
#include <assert.h>
#include "board.y.h"
#include "genmove.h"
#include "myrandom.h"
#include "connect.h"
#include "move.h"

void init_to_zero(int A[], int n) { int j;
  for (j=0; j<n; j++) A[j] = 0;
}

float ratio(int n, int d) {
  return (d==0) ? INFINITY: (float)n/(float)d;
}

float score(int wins, int opt_wins, int sum_lengths, int opt_sum_lengths) {
// score is zero-sum
  if (wins==0) return -INFINITY;
  if (opt_wins==0) return INFINITY;
  return -.5 + (float)wins/(float)ROLLOUTS+ //monte carlo win prob
         (float)opt_sum_lengths/(float)opt_wins- 
	 (float)sum_lengths/(float)wins;
}

int index_of_max(int A[], int n) { int j,max_psn=0;
  for (j=1; j<n; j++)
    if (A[j]>A[max_psn])
      max_psn = j;
  return max_psn;
}

int init_available(int G[], int A[]) {int r,c,psn; int j=0;
// put locations of available moves in A, return how many
  for (r = 0; r<N; r++)
    for (c = 0; c < N-r; c++) {
      psn = fatten(r,c);
      if (EMPTY_CELL==G[psn])
        A[j++] = psn;
      }
  assert(j!=0); // should always be an available move
  return j;
}

int rand_move(int G[]) { int Avail[TotalCells];
  int count = init_available(G,Avail);
  return Avail[myrand(count)];
}

int monte_carlo(int stone, struct myboard* B, Bool_t accelerate) { 
  // accelerate? winners   sublist A[0             .. end_winners]
  //             remainder sublist A[end_winners+1 ..  TotalCells]
  int j,k,z,count,turn;  int bd_set;
  int colorScore[2] = {0,0};  // black, white
  int wins[TotalGBCells], A[TotalCells];  // A "available"
  int winsBW[2][TotalGBCells];
  int win_length[2] = {0,0}; // sum (over wins) of stones in win
  struct myboard local;
  count = init_available(B->board,A); assert(count!=0);
  init_to_zero(wins,TotalGBCells);
  init_to_zero(winsBW[0],TotalGBCells);
  init_to_zero(winsBW[1],TotalGBCells);
  int end_winners = -1; int just_won = -1;
  shuffle_interval(A,0,count-1);
  for (j=0; j<ROLLOUTS; j++) {  //printf("j %d\n",j);
    local = *B;
    printf("starting rollout %d\n",j);
    if (j==2) show_Yboard(local.board);
    if (!accelerate) {
      shuffle_interval(A,0,count-1);
    }
    else { // if accelerate
      if (just_won > end_winners) { // add to winners; shuffle remainders
        ++end_winners;
        swap_int(&A[end_winners], &A[just_won]);
        shuffle_interval(A,end_winners+1,count-1);
      }
      shuffle_interval(A,0,end_winners); //shuffle winners
    }
    // common code
    turn = other_color(stone); k=0; bd_set = BORDER_NONE; 
    while (bd_set != BORDER_ALL) {
      turn = other_color(turn);
      assert(local.board[A[k]]==EMPTY_CELL);
      bd_set = make_move(turn,A[k],&local,TRUE);
      // played into oppt miai?
      int resp = local.reply[ndx(other_color(turn))][A[k]];
      if (resp!=A[k]) { // prep for autorespond on next move
        if (j==0) {
          printf("prep %c autoresp %c%d\n",
            emit(other_color(turn)),alphaCol(resp),numerRow(resp));
          show_Yboard(local.board);
          show_miai(local.reply[ndx(other_color(turn))]);
          show_miai(local.reply[ndx(turn)]);
          assert(local.board[resp]==EMPTY_CELL);
        }
        for (z=k+1; A[z]!=resp; z++) ;
        if (z>=TotalCells) {
          show_Yboard(local.board);
          show_miai(local.reply[ndx(other_color(turn))]);
          show_miai(local.reply[ndx(turn)]);
          assert(local.board[resp]==EMPTY_CELL);
        }
        assert(z<TotalCells);
        assert((k+1)<TotalCells);
        swap_int(&A[k+1],&A[z]);
        printf("swapped, A[%d] now %d, A[%d] now %d\n",
          k+1,A[k+1],z,A[z]);
      }
      k++;
    }
    printf("just won\n");
    just_won = k-1;
    colorScore[ndx(turn)]++;
    win_length[ndx(turn)] +=k;
    wins[A[just_won]]++;
    winsBW[ndx(turn)][A[just_won]]++;
  }
  //printf("b %d w %d\n",colorScore[0],colorScore[1]);
  assert(ROLLOUTS == colorScore[0]+colorScore[1]);
  int mywins = colorScore[stone-1];
  int owins  = colorScore[other_color(stone)-1];
  int mysum = win_length[stone-1];
  int osum = win_length[other_color(stone)-1];
  printf("%c wins %.2f   ", emit(stone),(float)mywins/(float)ROLLOUTS);
  printf("length %2.2f (oppt %2.2f)   score %2.2f \n\n",
    ratio(mysum,mywins), ratio(osum,owins), score(mywins,owins,mysum,osum));
  show_inner_Y(wins); //show_inner_Y(winsBW[0]); show_inner_Y(winsBW[1]);
  return index_of_max(wins,TotalGBCells);
}
