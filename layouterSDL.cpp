#include "layouterSDL.h"

typedef struct
{
  // set to the origin of the pixel
  uint32_t *pixels;

  // final bounding check
  uint32_t *first_pixel, *last_pixel;

  // format information
  int32_t pitch;
  uint32_t rshift;
  uint32_t gshift;
  uint32_t bshift;
  uint32_t ashift;

  uint8_t r, g, b, a;

} spanInfo;

void spanner(int y, int count, const FT_Span* spans, void *user)
{
  spanInfo *baton = (spanInfo *)user;

  uint32_t *scanline = baton->pixels - y * baton->pitch;

  if (scanline >= baton->first_pixel)
  {
    for (int i = 0; i < count; i++)
    {
      uint32_t *start = scanline + spans[i].x;

      if (start + spans[i].len >= baton->last_pixel)
        return;

      uint8_t alpha = (spans[i].coverage * baton->a) / 255;

      for (int x = 0; x < spans[i].len; x++)
      {
        uint8_t pr = *start >> baton->rshift;
        uint8_t pg = *start >> baton->gshift;
        uint8_t pb = *start >> baton->bshift;

        pr = ((255-alpha)*pr + (alpha*baton->r))/255;
        pg = ((255-alpha)*pg + (alpha*baton->g))/255;
        pb = ((255-alpha)*pb + (alpha*baton->b))/255;

        *start = (pr << baton->rshift) | (pg << baton->gshift) | (pb << baton->bshift);
        start++;

        if (start >= baton->last_pixel) return;
      }
    }
  }
}

void showLayoutSDL(const textLayout_c & l, int sx, int sy, SDL_Surface * s)
{
  /* set up rendering via spanners */
  spanInfo span;
  span.pixels = NULL;
  span.first_pixel = (uint32_t*)s->pixels;
  span.last_pixel = (uint32_t *) (((uint8_t *) s->pixels) + s->pitch*s->h);
  span.pitch = s->pitch/4;
  span.rshift = s->format->Rshift;
  span.gshift = s->format->Gshift;
  span.bshift = s->format->Bshift;

  FT_Raster_Params ftr_params;
  ftr_params.target = 0;
  ftr_params.flags = FT_RASTER_FLAG_DIRECT | FT_RASTER_FLAG_AA;
  ftr_params.user = &span;
  ftr_params.black_spans = 0;
  ftr_params.bit_set = 0;
  ftr_params.bit_test = 0;
  ftr_params.gray_spans = spanner;


  /* render */
  for (auto & i : l.data)
  {
    switch (i.command)
    {
      case textLayout_c::commandData::CMD_GLYPH:

        span.pixels = (uint32_t *)(((uint8_t *) s->pixels) + (sy+i.y) * s->pitch) + (sx+i.x);
        span.r = i.r;
        span.g = i.g;
        span.b = i.b;
        span.a = i.a;

        i.font->outlineRender(i.glyphIndex, &ftr_params);

        break;

      case textLayout_c::commandData::CMD_RECT:

        SDL_Rect r;
        r.x = i.x+sx;
        r.y = i.y+sy;
        r.w = i.w;
        r.h = i.h;
        SDL_FillRect(s, &r, SDL_MapRGBA(s->format, i.r, i.g, i.b, i.a));
        break;
    }
  }
}

void showLayoutsSelf(int w, int h, const std::vector<layoutInfo_c> & data)
{
  /* Initialize our SDL window */
  if(SDL_Init(SDL_INIT_VIDEO) < 0)   {
    fprintf(stderr, "Failed to initialize SDL");
    return;
  }

  SDL_Surface *screen;
  screen = SDL_SetVideoMode(w, h, 32, SDL_SWSURFACE | SDL_DOUBLEBUF);

  /* Enable key repeat, just makes it so we don't have to worry about fancy
   * scanboard keyboard input and such */
  SDL_EnableKeyRepeat(300, 130);
  SDL_EnableUNICODE(1);

  /* Clear our surface */
  SDL_FillRect(screen, NULL, 0 );

  for (auto & a : data)
    showLayoutSDL(a.layout, a.sx, a.sy, screen);

  SDL_Flip(screen);

  /* Our main event/draw loop */
  int done = 0;

  while (!done)
  {

    /* Handle SDL events */
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_KEYDOWN:
        case SDL_QUIT:
          done = 1;
          break;
      }
    }

    SDL_Delay(150);
  }

  SDL_Quit();
}
