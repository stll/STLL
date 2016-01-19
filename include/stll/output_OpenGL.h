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
#ifndef STLL_LAYOUTER_OPEN_GL
#define STLL_LAYOUTER_OPEN_GL

/** \file
 *  \brief OpenGL output driver
 */

#include "layouterFont.h"
#include "glyphAtlas.h"
#include "gamma.h"

namespace STLL {

class TextLayout_c;

// V: OpenGL version minimum
// C: texture size used for glyphe cache
// G: gamma convertor
template <int V, int C, class G = Gamma_c<>>
class showOpenGL
{
  private:
    internal::GlyphAtlas_c cache;
    G gamma;

    // openGL texture it  TODO muss raus
    GLuint glTextureId = 0;
    uint32_t uploadVersion = 0;

  public:

    showOpenGL(void) : cache(C, C) {
      glGenTextures(1, &glTextureId);
      glBindTexture(GL_TEXTURE_2D, glTextureId);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexEnvi(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      gamma.setGamma(22);
    }

    ~showOpenGL(void)
    {
        glDeleteTextures(1, &glTextureId);
    }

    class imageDrawerOpenGL_c
    {
      public:
        virtual void draw(int32_t x, int32_t y, uint32_t w, uint32_t h, const std::string & url) = 0;
    };

    void showLayout(const TextLayout_c & l, int sx, int sy, SubPixelArrangement sp, imageDrawerOpenGL_c * images)
    {
      // make sure that all required glyphs are on our glyph texture, when necessary recreate it freshly
      // bind the texture
      for (auto & i : l.getData())
      {
        switch (i.command)
        {
          case CommandData_c::CMD_GLYPH:
            cache.getGlyph(i.font, i.glyphIndex, sp, i.blurr);
            break;
          case CommandData_c::CMD_RECT:
            if (i.blurr > 0)
              cache.getRect(i.w, i.h, sp, i.blurr);
            break;

          default:
            break;
        }
      }

      glBindTexture( GL_TEXTURE_2D, glTextureId );

      if (cache.getVersion() != uploadVersion)
      {
        uploadVersion = cache.getVersion();
        glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA, C, C, 0, GL_ALPHA, GL_UNSIGNED_BYTE, cache.getData() );
      }

      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      // output the layout
      for (auto & i : l.getData())
      {
        switch (i.command)
        {
          case CommandData_c::CMD_GLYPH:
            {
              glEnable(GL_TEXTURE_2D);
              auto pos = cache.getGlyph(i.font, i.glyphIndex, sp, i.blurr);
              glBegin(GL_QUADS);
              Color_c c = gamma.forward(i.c);
              glColor3f(c.r()/255.0, c.g()/255.0, c.b()/255.0);
              glTexCoord2f(1.0*(pos.pos_x-0.5)/C,           1.0*(pos.pos_y-0.5)/C);          glVertex3f(0.5+(sx+i.x)/64.0+pos.left-1,           0.5+(sy+i.y+32)/64-pos.top-1,          0);
              glTexCoord2f(1.0*(pos.pos_x-0.5+pos.width)/C, 1.0*(pos.pos_y-0.5)/C);          glVertex3f(0.5+(sx+i.x)/64.0+pos.left+pos.width-1, 0.5+(sy+i.y+32)/64-pos.top-1,          0);
              glTexCoord2f(1.0*(pos.pos_x-0.5+pos.width)/C, 1.0*(pos.pos_y-0.5+pos.rows)/C); glVertex3f(0.5+(sx+i.x)/64.0+pos.left+pos.width-1, 0.5+(sy+i.y+32)/64-pos.top+pos.rows-1, 0);
              glTexCoord2f(1.0*(pos.pos_x-0.5)/C,           1.0*(pos.pos_y-0.5+pos.rows)/C); glVertex3f(0.5+(sx+i.x)/64.0+pos.left-1,           0.5+(sy+i.y+32)/64-pos.top+pos.rows-1, 0);
              glEnd();
            }
            break;

          case CommandData_c::CMD_RECT:
            {
              if (i.blurr == 0)
              {
                glDisable(GL_TEXTURE_2D);
                Color_c c = gamma.forward(i.c);
                glBegin(GL_QUADS);
                glColor3f(c.r()/255.0, c.g()/255.0, c.b()/255.0);
                glVertex3f((sx+i.x    +32)/64+0.5, (sy+i.y    +32)/64+0.5, 0);
                glVertex3f((sx+i.x+i.w+32)/64+0.5, (sy+i.y    +32)/64+0.5, 0);
                glVertex3f((sx+i.x+i.w+32)/64+0.5, (sy+i.y+i.h+32)/64+0.5, 0);
                glVertex3f((sx+i.x    +32)/64+0.5, (sy+i.y+i.h+32)/64+0.5, 0);
                glEnd();
              }
              else
              {
                glEnable(GL_TEXTURE_2D);
                auto pos = cache.getRect(i.w, i.h, sp, i.blurr);
                glBegin(GL_QUADS);
                Color_c c = gamma.forward(i.c);
                glColor3f(c.r()/255.0, c.g()/255.0, c.b()/255.0);
                glTexCoord2f(1.0*(pos.pos_x-0.5)/C,           1.0*(pos.pos_y-0.5)/C);          glVertex3f(0.5+(sx+i.x)/64.0+pos.left-1,           0.5+(sy+i.y+32)/64-pos.top-1,          0);
                glTexCoord2f(1.0*(pos.pos_x-0.5+pos.width)/C, 1.0*(pos.pos_y-0.5)/C);          glVertex3f(0.5+(sx+i.x)/64.0+pos.left+pos.width-1, 0.5+(sy+i.y+32)/64-pos.top-1,          0);
                glTexCoord2f(1.0*(pos.pos_x-0.5+pos.width)/C, 1.0*(pos.pos_y-0.5+pos.rows)/C); glVertex3f(0.5+(sx+i.x)/64.0+pos.left+pos.width-1, 0.5+(sy+i.y+32)/64-pos.top+pos.rows-1, 0);
                glTexCoord2f(1.0*(pos.pos_x-0.5)/C,           1.0*(pos.pos_y-0.5+pos.rows)/C); glVertex3f(0.5+(sx+i.x)/64.0+pos.left-1,           0.5+(sy+i.y+32)/64-pos.top+pos.rows-1, 0);
                glEnd();
              }
            }
            break;

          case CommandData_c::CMD_IMAGE:
            if (images)
              images->draw(i.x+sx, i.y+sy, i.w, i.h, i.imageURL);
            break;
        }
      }
    }

    const uint8_t * getData(void) const { return cache.getData(); }
};

}

#endif
