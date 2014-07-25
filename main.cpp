#include "layouter.h"
#include "layouterSDL.h"

#define NUM_EXAMPLES 3

#include <hb.h>
#include <hb-ft.h>

/* tranlations courtesy of google */
const char *texts[NUM_EXAMPLES] = {
    "Test ЛенивыйVA",
    "Test كسول 123 الزنجبيل القط التعليقات",
//    "Test באתר 123 בנייה, העגורן הוא"
//    "Test الزنجبي",
//    "懶惰的姜貓",
};

// a trivial example

int main ()
{
  textStyleSheet_c styleSheet;

  styleSheet.language="ar-Arab";
  styleSheet.font("sans", "fonts/amiri-0.104/amiri-regular.ttf");
//  styleSheet.font("sans", "/usr/share/fonts/freefont/FreeSans.ttf");

  std::vector<layoutInfo_c> l;

  l.emplace_back(layoutInfo_c(layout(texts[1], styleSheet), 10, 80));

  showLayoutsSelf(800, 600, l);
}


#if 0
//a slightly more complex example
int main ()
{
  auto fontcache = std::make_shared<fontCache_c>();
  std::vector<textStyleSheet_c> styleSheets;
  std::vector<fontFamilyCollection_c> fontFamilies;

  textStyleSheet_c ss;
  fontFamilyCollection_c c;

  styleSheets.push_back(ss);
  c["sans"] = std::make_shared<fontFamily_c>(fontcache);
  c["sans"]->addFont("fonts/DejaVuSerif.ttf");
  fontFamilies.push_back(c);

  ss.language="ar-Arab";
  styleSheets.push_back(ss);
  c["sans"] = std::make_shared<fontFamily_c>(fontcache);
  c["sans"]->addFont("fonts/amiri-0.104/amiri-regular.ttf");
  fontFamilies.push_back(c);

  ss.language="ch-Hani";
  styleSheets.push_back(ss);
  c["sans"] = std::make_shared<fontFamily_c>(fontcache);
  c["sans"]->addFont("fonts/fireflysung-1.3.0/fireflysung.ttf");
  fontFamilies.push_back(c);

  std::vector<textLayout_c> layouts;

  layouts.emplace_back(layout(texts[0], styleSheets[0], fontFamilies[0]));
  layouts.emplace_back(layout(texts[1], styleSheets[1], fontFamilies[1]));
  layouts.emplace_back(layout(texts[2], styleSheets[2], fontFamilies[2]));

  /** Setup our SDL window **/
  int width      = 800;
  int height     = 600;
  int videoFlags = SDL_SWSURFACE | SDL_DOUBLEBUF;
  int bpp        = 32;

  /* Initialize our SDL window */
  if(SDL_Init(SDL_INIT_VIDEO) < 0)   {
    fprintf(stderr, "Failed to initialize SDL");
    return -1;
  }

  SDL_WM_SetCaption("\"Simple\" SDL+FreeType+HarfBuzz Example", "\"Simple\" SDL+FreeType+HarfBuzz Example");

  SDL_Surface *screen;
  screen = SDL_SetVideoMode(width, height, bpp, videoFlags);

  /* Enable key repeat, just makes it so we don't have to worry about fancy
   * scanboard keyboard input and such */
  SDL_EnableKeyRepeat(300, 130);
  SDL_EnableUNICODE(1);

  /* Create an SDL image surface we can draw to */
  SDL_Surface *sdl_surface = SDL_CreateRGBSurface (0, width, height, 32, 0,0,0,0);

  /* Clear our surface */
  SDL_FillRect( sdl_surface, NULL, 0 );
  SDL_LockSurface(sdl_surface);

  SDL_UnlockSurface(sdl_surface);

  showLayoutSDL(layouts[0], 10, 80 + 0 * 105, sdl_surface);
  showLayoutSDL(layouts[1], 10, 80 + 1 * 105, sdl_surface);
  showLayoutSDL(layouts[2], 10, 80 + 2 * 105, sdl_surface);

  /* Blit our new image to our visible screen */

  SDL_BlitSurface(sdl_surface, NULL, screen, NULL);
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

  SDL_FreeSurface(sdl_surface);

  SDL_Quit();
}
#endif