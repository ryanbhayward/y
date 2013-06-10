#include <cstdio>
#include <cstdlib>
#include <cctype>
#include "ui.h"

static const char THISMOVE_CH    = '!';  // used for specified moves

void displayHelp() {
  printf("\033[2J\033[1;1H");  // clear screen and reset cursor
  printf(" play Y     (2012-3 RBH + JP + BA + JH)\n\n");  
  printf("   %c a3           (black a3)\n",BLK_CH);
  printf("   %c %c       (white genmove)\n",WHT_CH,GENMOVE_CH);
  printf("   %c 3 4      (play 3x4 hex)\n",PLAYHEX_CH);
  printf("   %c                  (undo)\n",UNDO_CH);
  printf("[return]              (quit)\n\n");
}

bool valid_psn(int psn) {
  return ((psn > 0)&&(psn <= N)) ? true:false;
}

bool parse_int(char& ch, int& y) { 
  y = 0;
  while ((ch >= '0') && (ch <= '9')) {
    y = 10*y+ ch-'0';
    ch = getchar();
  }
  return (y>0);
}

int board_format(int x, int y) {
  return (y+GUARDS-1)*(Np2G)+x+GUARDS-1;
}

void nextEOL(char& ch) {
  while (ch != '\n') ch = getchar();
}

void parseQ(char& cmd, char ch, bool& q) {
  cmd = ch; q = true;
}

bool expect(char ch0, char ch1) {
  return (ch0==ch1);
}

void parseU(char& cmd, char ch, bool& q) {
  cmd = ch; ch = getchar();
  q = expect(ch,'\n');
  if (!q) {
    printf("expected EOL after %c\n",cmd);
    nextEOL(ch);
  }
}

void parsePH(char& cmd, char ch, bool& ok, int& x, int& y) {
  cmd = ch; ch = getchar(); 
  ok = expect   (ch,' ');
  if (ok) {
    ch = getchar();
    ok = parse_int(ch, x);
  }
  if (ok) {
    ok = expect(ch,' ');
  }
  if (ok) {
    ch = getchar();
    ok = parse_int(ch, y);
  }
  if (ok) {
    ok = expect(ch,'\n');
  }
  if (!ok) {
     printf("expected h integer integer\n");
     nextEOL(ch);
  }
}

void parseGenmove(char& cmd, char ch, bool& ok) { 
  cmd = ch;
  ch = getchar();
  ok = expect(ch,'\n');
  if (!ok) {
    nextEOL(ch);
    printf("expected EOL after %c\n",cmd);
  }
}

void parseGivenMv(char& cmd, char ch, bool& ok, int& lcn) { int x,y;
  cmd = THISMOVE_CH;
  ok = islower(ch);
  if (ok) {
    x = 1 + ch - 'a';
    ch = getchar();
    ok = parse_int(ch,y);
  }
  if (ok) 
    ok = expect(ch,'\n');
  if (ok) {
    ok = (valid_psn(x)&&valid_psn(y));
    if (ok)
      lcn = board_format(x,y);
  }
  if (!ok) {
    nextEOL(ch);
    printf("what move is that?\n");
  }
}

void parseMove(char& cmd, char ch, bool& ok, int& s, int& lcn) { 
  if (ch==BLK_CH) s = BLK;
  else            s = WHT;
  ch = getchar();
  ok = expect(ch,' ');
  if (ok) {
    ch = getchar(); 
    if (ch==GENMOVE_CH) 
      parseGenmove(cmd, ch, ok);
    else 
      parseGivenMv(cmd, ch, ok, lcn); // s already assigned
  }
}

void getCommand(char& cmd, int& s, int& lcn) { 
  bool ok = false; 
  do {
    printf("\n=> ");
    char ch = getchar();
    switch (ch) {
      case QUIT_CH:       parseQ (cmd,ch,ok); break;
      case UNDO_CH:       parseU (cmd,ch,ok); break;
      case PLAYHEX_CH:    parsePH(cmd,ch,ok,s,lcn); break;
      case BLK_CH:
      case WHT_CH:        parseMove(cmd,ch,ok,s,lcn); break; 
      default: nextEOL(ch); printf("sorry, what?\n");
    } 
  } while (!ok);
}
