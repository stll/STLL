#ifndef __LAYOUTER_SDL__
#define __LAYOUTER_SDL__

#include "layouter.h"

#include <SDL.h>

#include <vector>

void showLayoutSDL(const textLayout_c & l, int sx, int sy, SDL_Surface * s);

class layoutInfo_c
{
  public:

    layoutInfo_c(textLayout_c l, int x, int y) : layout(l), sx(x), sy(y) { }

    textLayout_c layout;
    int sx, sy;
};

void showLayoutsSelf(int w, int h, const std::vector<layoutInfo_c> & data);

#endif
