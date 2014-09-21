/*
 * STLL Simple Text Layouting Library
 *
 * STLL is the legal property of its developers, whose
 * names are listed in the COPYRIGHT file, which is included
 * within the source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "layouterFont.h"
#include "layouterSDL.h"
#include "layouterFont.h"
#include "layouterXMLSaveLoad.h"

#include <pugixml.hpp>

using namespace STLL;


int main(int argv, char ** args)
{
  pugi::xml_document doc;

  auto res = doc.load_file(args[1]);

  if (!res)
  {
    printf("%s\n", (std::string("oopsi loading file...") + res.description()).c_str());
    return 1;
  }

  auto c = std::make_shared<fontCache_c>();
  auto l = loadLayoutFromXML(doc.child("layout"), c);

  if(SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    fprintf(stderr, "Failed to initialize SDL");
    return 1;
  }

  SDL_Surface *screen = SDL_SetVideoMode(l.getRight()/64, l.getHeight()/64, 32, SDL_SWSURFACE | SDL_DOUBLEBUF);

  if(!screen)
  {
    fprintf(stderr, "Failed to create SDL window");
    return 1;
  }

  /* Enable key repeat, just makes it so we don't have to worry about fancy
   * scanboard keyboard input and such */
  SDL_EnableKeyRepeat(300, 130);
  SDL_EnableUNICODE(1);

  /* Clear our surface */
  SDL_FillRect(screen, NULL, 0 );

  /* draw a grid with 10x10 pixel squares */
  {
    SDL_Rect r;
    for (int x = 0; x < 1+l.getRight()/640; x++)
      for (int y = 0; y < 1+l.getHeight()/640; y++)
      {
        if ((x + y) % 2)
        {
          r.x = x*10;
          r.y = y*10;
          r.w = r.h = 10;
          SDL_FillRect(screen, &r, SDL_MapRGBA(screen->format, 50, 50, 50, 255));
        }
      }
  }

  showLayoutSDL(l, 0, 0, screen);

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
