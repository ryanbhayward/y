#include "connect.h"

int my_find(int Parents[], int x) {
  int px = Parents[x];
  if (x==px) return x;
  int gx = Parents[px];
  if (px==gx) return px;
  Parents[x] = gx; //printf("find parent %d <- %d\n",x,gx);
  return my_find(Parents, gx);
}

int my_union(int Parents[], int x, int y) {
  //printf("union %d %d\n",x,y);
  //int xRoot = my_find(Parents, x);
  //int yRoot = my_find(Parents, y);
  //Parents[xRoot] = yRoot; //printf("parent %d <- %d\n",xRoot,yRoot);
  //return yRoot;
  Parents[x] = y; //printf("parent %d <- %d\n",xRoot,yRoot);
  return y;
}
