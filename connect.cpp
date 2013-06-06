#include "connect.h"

int Find(int Parents[], int x) {
  int px = Parents[x];
  if (x==px) return x;
  int gx = Parents[px];
  if (px==gx) return px;
  Parents[x] = gx; //printf("Find parent %d <- %d\n",x,gx);
  return Find(Parents, gx);
}

int Union(int Parents[], int x, int y) {
  //printf("Union %d %d\n",x,y);
  //int xRoot = Find(Parents, x);
  //int yRoot = Find(Parents, y);
  //Parents[xRoot] = yRoot; //printf("parent %d <- %d\n",xRoot,yRoot);
  //return yRoot;
  Parents[x] = y; //printf("parent %d <- %d\n",xRoot,yRoot);
  return y;
}
