#include "layouterSDL.h"

typedef struct _spanner_baton_t {
    /* rendering part - assumes 32bpp surface */
    uint32_t *pixels; // set to the glyph's origin.
    uint32_t *first_pixel, *last_pixel; // bounds check
    uint32_t pitch;
    uint32_t rshift;
    uint32_t gshift;
    uint32_t bshift;
    uint32_t ashift;

    uint8_t r, g, b;

} spanner_baton_t;

/* This spanner does read/modify/write, trading performance for accuracy.
   The color here is simply half coverage value in all channels,
   effectively mid-gray.
   Suitable for when artifacts mostly do come up and annoy.
   This might be optimized if one does rmw only for some values of x.
   But since the whole buffer has to be rw anyway, and the previous value
   is probably still in the cache, there's little point to. */
void spanner(int y, int count, const FT_Span* spans, void *user)
{
  spanner_baton_t *baton = (spanner_baton_t *) user;

  uint32_t *scanline = baton->pixels - y * ( (int) baton->pitch / 4 );

  if (scanline >= baton->first_pixel)
  {
    for (int i = 0; i < count; i++)
    {
      uint32_t color =
      ((spans[i].coverage*baton->r/256) << baton->rshift) |
      ((spans[i].coverage*baton->g/256) << baton->gshift) |
      ((spans[i].coverage*baton->b/256) << baton->bshift);

      uint32_t *start = scanline + spans[i].x;

      if (start + spans[i].len >= baton->last_pixel)
        return;

      for (int x = 0; x < spans[i].len; x++)
        *start++ |= color;
    }
  }
}

void showLayoutSDL(const textLayout_c & l, int sx, int sy, SDL_Surface * s)
{
  /* set up rendering via spanners */
  spanner_baton_t stuffbaton;
  stuffbaton.pixels = NULL;
  stuffbaton.first_pixel = (uint32_t*)s->pixels;
  stuffbaton.last_pixel = (uint32_t *) (((uint8_t *) s->pixels) + s->pitch*s->h);
  stuffbaton.pitch = s->pitch;
  stuffbaton.rshift = s->format->Rshift;
  stuffbaton.gshift = s->format->Gshift;
  stuffbaton.bshift = s->format->Bshift;


  FT_Raster_Params ftr_params;
  ftr_params.target = 0;
  ftr_params.flags = FT_RASTER_FLAG_DIRECT | FT_RASTER_FLAG_AA;
  ftr_params.user = &stuffbaton;
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

        stuffbaton.pixels = (uint32_t *)(((uint8_t *) s->pixels) + (sy+i.y) * s->pitch) + (sx+i.x);
        stuffbaton.r = i.r;
        stuffbaton.g = i.g;
        stuffbaton.b = i.b;

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
