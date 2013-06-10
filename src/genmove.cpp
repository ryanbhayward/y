#include <cassert>
#include <cstdio>
#include <cstring>
#include "connect.h"
#include "genmove.h"
#include "move.h"
#include "shuff.h"
#include "node.h"

void init_to_zero(int A[], int n) { int j;
  for (j=0; j<n; j++) A[j] = 0;
}

float ratio(int n, int d) {
  return (d==0) ? Y_INFINITY: (float)n/(float)d;
}

float score(int wins, int opt_wins, int sum_lengths, int opt_sum_lengths) {
// score is zero-sum
  if (wins==0) return -Y_INFINITY;
  if (opt_wins==0) return Y_INFINITY;
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

int rand_move(Board &B) { 
  Playout pl(B);
  return pl.Avail[myrand(0,pl.numAvail)];
}

int uct_move(Board& B, int s, bool useMiai) {
  Node root; // init
  root.expand(B,s);
  for (int j =0; j<ROLLOUTS; j++) {
    Board L = B;
    root.uct_playout(L,s,useMiai);
    // Check the root for immediate wins
    for (int i = 0; i < root.numChildren; i++) {
      if(root.children[i].node.proofStatus == PROVEN_LOSS) {
	printf("Found win: "); prtLcn(root.children[i].lcn); printf("\n");
	printf("Found in %d iterations.\n", j);
	return root.children[i].lcn;
      }
    }
  }
  int childWins[TotalGBCells];
  int childVisits[TotalGBCells];
  int uctEval[TotalGBCells];
  int proofStatus[TotalGBCells];
  memset(childWins,0,sizeof(childWins));
  memset(childVisits,0,sizeof(childVisits));
  memset(uctEval,0,sizeof(uctEval));
  memset(proofStatus, 0, sizeof(proofStatus));
  for (int j=0; j < root.numChildren; j++) {
    Child& c = root.children[j];
    childWins[c.lcn] = c.node.stat.w;
    childVisits[c.lcn] = c.node.stat.n;
    uctEval[c.lcn] = 1000*root.ucb_eval(c.node);
    proofStatus[c.lcn] = c.node.proofStatus;
  }
  printf("childWins  childVisits uctEval proofStatus\n");
  showYcore(childWins);
  showYcore(childVisits);
  showYcore(uctEval);
  showYcore(proofStatus);
  return root.bestMove();
}

int monte_carlo(Board& B, int s, bool useMiai, bool accelerate) { 
  // accelerate? winners   sublist A[0             .. end_winners]
  //             remainder sublist A[end_winners+1 ..  TotalCells]
  Board local; Playout pl(B); assert(pl.numAvail!=0);
  int end_winners = -1; int just_won = -1;
  shuffle_interval(pl.Avail,0,pl.numAvail-1);
  for (int j=0; j<ROLLOUTS; j++) { 
    local = B;
    if (!accelerate) {
      shuffle_interval(pl.Avail,0,pl.numAvail-1);
    }
    else { // if accelerate
      if (just_won > end_winners) { // add to winners; shuffle remainders
        std::swap(pl.Avail[++end_winners], pl.Avail[just_won]);
        shuffle_interval(pl.Avail,end_winners+1,pl.numAvail-1);
      }
      shuffle_interval(pl.Avail,0,end_winners); //shuffle winners
    }
    // common code
    int turn = s;
    pl.single_playout(turn, just_won, useMiai);
    if (just_won==1) { // opponent wins with first opponent move
      //printf("jw %d",just_won); prtLcn(A[0]);prtLcn(A[1]);printf("!?\n");
      pl.wins[pl.Avail[0]] = 0; /// HACK
    }
    pl.colorScore[ndx(turn)]++;
    pl.win_length[ndx(turn)] +=just_won+1;
    pl.wins[pl.Avail[just_won]]++;
    pl.winsBW[ndx(turn)][pl.Avail[just_won]]++;
  }
  assert(ROLLOUTS == pl.colorScore[0]+pl.colorScore[1]);
  int mywins = pl.colorScore[ndx(s)];
  int owins  = pl.colorScore[ndx(oppnt(s))];
  int mysum = pl.win_length[ndx(s)];
  int osum = pl.win_length[ndx(oppnt(s))];
  printf("%c wins %.2f   ", ColorToChar(s),(float)mywins/(float)ROLLOUTS);
  printf("length %2.2f (oppt %2.2f)   score %2.2f \n\n",
    ratio(mysum,mywins), ratio(osum,owins), score(mywins,owins,mysum,osum));
  //showYcore(pl.wins); 
  showYcore(pl.winsBW[ndx(s)]); 
  showYcore(pl.winsBW[ndx(oppnt(s))]); 
  return index_of_max(pl.wins,TotalGBCells);
}
