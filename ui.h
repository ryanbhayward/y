#ifndef UI_H
#define UI_H
#include "board.y.h"

//#define HELP_CH '?'
#define GENMOVE_CH '?'
#define PLAY_CH 'p'
#define UNDO_CH 'u'
#define QUIT_CH '\n'

extern void displayHelp() ;
extern void welcome() ;
extern void help() ;
extern void interact() ;
extern void getCommand(int *stone, int *lcn, char* cmd);
#endif
