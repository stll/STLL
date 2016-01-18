// includes for the layouter and the SDL output functions, if you want to
// output the text using a different graphics library, you need a different
// include here
#include <stll/layouter.h>
#include <stll/output_SDL.h>

// all STLL symbols are in the namespace STLL
using namespace STLL;

int main()
{
  // create a font cache to get fonts. the font cache will take care of
  // opening font files and providing it to the application. you need to
  // keep the object alive until you are done with the layouting and the output
  // usually you will have one font cache per application
  FontCache_c fc;

  // setup the attributes for the text you want to output
  // we use a font from the fontcache and the color white
  // we also use the English language
  // keep in mind that the font size is given in units of 1/64th, so
  // multiply by 64
  CodepointAttributes_c attr;
  attr.font = fc.getFont(FontResource_c("tests/FreeSans.ttf"), 20*64);
  attr.c = Color_c(255, 255, 255);
  attr.lang = "en";

  // setup the attribute index. This index assigns attributes to
  // all the characters in your text, the way we create it here
  // will setup the index in such a way, that _all_ characters will
  // have the given attribute
  AttributeIndex_c ai(attr);

  // just to show how: make the 6th glyph (the W) red
  attr.c = Color_c(255, 0, 0);
  ai.set(6, attr);

  // layouting properties, the default is good enough for us right now
  LayoutProperties_c prop;

  // now layout the text "Hello World" with the given attributes, put
  // it into a rectangle of width 200 pixel and use our layouting properties
  // the text needs to be utf-32 encoded for this function
  auto layout = layoutParagraph(U"Hello World", ai, RectangleShape_c(200*64), prop);

  // that is it, now you can output the layout, using one of the Output
  // driver functions, e.g. using the SDL driver you can output the text
  // at position 20, 20 on the given surface (also in units of 1/64th)
  SDL_Surface *screen = SDL_SetVideoMode(800, 600, 32, SDL_SWSURFACE | SDL_DOUBLEBUF);

  // create the output object, with glyph cache and all local storage, keep this
  // object around for the lifetime of your program
  showSDL<> show;
  show.showLayout(layout, 20*64, 20*64, screen);
  SDL_Flip(screen);
  sleep(10);

  // now you can get rid of the cache, if you want
}
