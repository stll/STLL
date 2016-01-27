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
#include "color.h"

#include "internal/glyphAtlas.h"
#include "internal/gamma.h"
#include "internal/ogl_shader.h"

namespace STLL {

/** \brief a class to output layouts using OpenGL
 *
 * To output layouts using this class, create an object of it and then
 * use the showLayout Function to output the layout.
 *
 * The class contains a glyph cache in form of an texture atlas. Once this
 * atlas if full, things available will be output and then the atlas will be cleared
 * and repopulated for the next section of the output. This will slow down output
 * considerably, so choose the size wisely. The atlas will be destroyed once the
 * class is destroyed. Things to consider:
 * - using sub pixel placement triples the space requirements for the glyphs
 * - blurring adds quite some amount of space around the glyphs, but as soon as you blurr the
 *   sub pixel placement will not be used as you will not see the difference anyways
 * - normal rectangles will not go into the cache, but blurres ones will
 * - so in short avoid blurring
 *
 * As OpenGL required function loaders and does not provide a header with all available
 * functionality you will need to include the proper OpenGL before including the header file
 * for this class.
 *
 * Gamma correct output is not handled by this class directly. You need to activate the sRGB
 * property for the target that this paints on
 *
 * \tparam C size of the texture cache. The cache is square C time C pixels.
 * \tparam G the gamma calculation function, if you use sRGB output... normally you don't need
 * to change this, keep the default
 */
template <int C, class G = internal::Gamma_c<>>
class showOpenGL
{
  private:
    internal::GlyphAtlas_c cache;
    G gamma;

    GLuint glTextureId = 0;     // OpenGL texture id
    uint32_t uploadVersion = 0; // a counter changed each time the texture changes to know when to update
    uint32_t atlasId = 1;

    internal::OGL_Program_c program;
    GLuint vertexBuffer;

    class vertex
    {
    public:
      GLfloat x, y;   // position
      GLfloat u, v;   // texture
      uint8_t r, g, b, a; // colour

      vertex (GLfloat _x, GLfloat _y, GLfloat _u, GLfloat _v, Color_c c) :
      x(_x), y(_y), u(_u), v(_v), r(c.r()), g(c.g()), b(c.b()), a(c.a()) {}
    };


    void drawBuffers(SubPixelArrangement sp, size_t s, int sx, int sy)
    {
      glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, x));
      glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, u));
      glEnableVertexAttribArray(2); glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, r));

      program.setUniform("offset", sx/64.0, sy/64);

      switch (sp)
      {
        default:
        case SUBP_NONE:
          program.setUniform("ashift", 0);
          glDrawArrays(GL_QUADS, 0, s);
          break;

        case SUBP_RGB:
          glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE); program.setUniform("ashift", -1.0f/C); glDrawArrays(GL_QUADS, 0, s);
          glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE); program.setUniform("ashift", -0.0f/C); glDrawArrays(GL_QUADS, 0, s);
          glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_FALSE); program.setUniform("ashift",  1.0f/C); glDrawArrays(GL_QUADS, 0, s);
          glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
          break;

        case SUBP_BGR:
          glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_FALSE); program.setUniform("ashift", -1.0f/C); glDrawArrays(GL_QUADS, 0, s);
          glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE); program.setUniform("ashift", -0.0f/C); glDrawArrays(GL_QUADS, 0, s);
          glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE); program.setUniform("ashift",  1.0f/C); glDrawArrays(GL_QUADS, 0, s);
          glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
          break;
      }    }

  public:

    /** \brief class to store a layout in a cached format for even faster repaint */
    class DrawCache_c
    {
    public:

      GLuint vBuffer;
      size_t elements;
      uint32_t atlasId;

      ~DrawCache_c(void)
      {
        glDeleteBuffers(1, &vBuffer);
      }

      DrawCache_c(void) : vBuffer(0), elements(0), atlasId(0) {}

      DrawCache_c(const DrawCache_c &) = delete;

      DrawCache_c(DrawCache_c && orig)
      {
        vBuffer = orig.vBuffer; orig.vBuffer = 0;
        elements = orig.elements; orig.elements = 0;
        atlasId = orig.atlasId; orig.atlasId = 0;
      }

      void operator=(DrawCache_c && orig)
      {
        vBuffer = orig.vBuffer; orig.vBuffer = 0;
        elements = orig.elements; orig.elements = 0;
        atlasId = orig.atlasId; orig.atlasId = 0;
      }
    };

    /** \brief constructor */
    showOpenGL(void) : cache(C, C) {
      glActiveTexture(GL_TEXTURE0);
      glGenTextures(1, &glTextureId);
      glBindTexture(GL_TEXTURE_2D, glTextureId);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexEnvi(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      gamma.setGamma(22);

      program.attachShader(GL_FRAGMENT_SHADER, "110",
        "uniform sampler2D texture;"

        "void main()"
        "{"
        "  vec2 uv = gl_TexCoord[0].xy;"
        "  vec4 current = texture2D(texture, uv);"
        "  gl_FragColor = vec4(gl_Color.r, gl_Color.g, gl_Color.b, current.a);"
        "}"
      );

      program.attachShader(GL_VERTEX_SHADER, "110",
        "uniform float width;"
        "uniform float height;"
        "uniform float ashift;"
        "uniform vec2 offset;"

        "attribute vec2 vertex;"
        "attribute vec4 color;"
        "attribute vec2 tex_coord;"
        "void main()"
        "{"
        "  gl_FrontColor = vec4(color.r/255.0, color.g/255.0, color.b/255.0, color.a/255.0);"
        "  gl_TexCoord[0].xy = vec2(tex_coord.x+ashift, tex_coord.y);"
        "  gl_Position = vec4((vertex.x-width+offset.x)/width, 1.0-(vertex.y+offset.y)/height, 0, 1.0);"
        "}"
      );

      program.bindAttributeLocation(0, "vertex");
      program.bindAttributeLocation(1, "tex_coord");
      program.bindAttributeLocation(2, "color");

      program.link();

      program.setUniform("texture", 0);

      glGenBuffers(1, &vertexBuffer);
    }

    ~showOpenGL(void)
    {
        glDeleteTextures(1, &glTextureId);
        glDeleteBuffers(1, &vertexBuffer);
    }

    /** \brief helper class used to draw images */
    class imageDrawerOpenGL_c
    {
      public:
        virtual void draw(int32_t x, int32_t y, uint32_t w, uint32_t h, const std::string & url) = 0;
    };


    /** \brief paint the layout
     *
     * \param l the layout to draw
     * \param sx x position on the target surface in 1/64th pixels
     * \param sy y position on the target surface in 1/64th pixels
     * \param sp which kind of sub-pixel positioning do you want?
     * \param images a pointer to an image drawer class that is used to draw the images, when you give
     *                a nullptr here, no images will be drawn
     * \return if true then the cache had to be cleared... which might hint at a problem with your cache
     * size
     */
    void showLayout(const TextLayout_c & l, int sx, int sy, SubPixelArrangement sp,
                    imageDrawerOpenGL_c * images = nullptr, DrawCache_c * dc = nullptr)
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, glTextureId );
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      if (dc && dc->atlasId == atlasId)
      {
        glBindBuffer(GL_ARRAY_BUFFER, dc->vBuffer);

        drawBuffers(sp, dc->elements, sx, sy);
        return;
      }

      const auto & dat = l.getData();
      size_t i = 0;
      bool cleared = false;

      while (i < dat.size())
      {
        size_t j = i;

        // make sure that there is a small completely filled rectangle
        // used for drawing filled rectangles
        cache.getRect(640, 640, SUBP_NONE, 0);

        while (j < dat.size())
        {
          auto & ii = dat[j];

          bool found = true;

          switch (ii.command)
          {
            case CommandData_c::CMD_GLYPH:
              // when subpixel placement is on we always create all 3 required images
              found &= (bool)cache.getGlyph(ii.font, ii.glyphIndex, sp, ii.blurr);
              break;
            case CommandData_c::CMD_RECT:
              if (ii.blurr > 0)
                found &= (bool)cache.getRect(ii.w, ii.h, sp, ii.blurr);
              break;

            default:
              break;
          }

          if (!found) break;

          j++;
        }

        if (cache.getVersion() != uploadVersion)
        {
          uploadVersion = cache.getVersion();
          glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA, C, C, 0, GL_ALPHA, GL_UNSIGNED_BYTE, cache.getData() );
        }

        std::vector<vertex> vb;
        vb.reserve(dat.size()*4);

        size_t k = i;

        while (k < j)
        {
          auto & ii = dat[k];

          switch (ii.command)
          {
            case CommandData_c::CMD_GLYPH:
              {
                auto pos = cache.getGlyph(ii.font, ii.glyphIndex, sp, ii.blurr).value();
                Color_c c = gamma.forward(ii.c);

                if ((sp == SUBP_RGB || sp == SUBP_BGR) && (ii.blurr <= cache.blurrmax))
                {
                  vb.push_back(vertex((ii.x)/64.0+pos.left,               (ii.y+32)/64-pos.top,         1.0*(pos.pos_x)/C,           1.0*(pos.pos_y)/C,          c));
                  vb.push_back(vertex((ii.x)/64.0+pos.left+pos.width/3.0, (ii.y+32)/64-pos.top,         1.0*(pos.pos_x+pos.width)/C, 1.0*(pos.pos_y)/C,          c));
                  vb.push_back(vertex((ii.x)/64.0+pos.left+pos.width/3.0, (ii.y+32)/64-pos.top+pos.rows,1.0*(pos.pos_x+pos.width)/C, 1.0*(pos.pos_y+pos.rows)/C, c));
                  vb.push_back(vertex((ii.x)/64.0+pos.left,               (ii.y+32)/64-pos.top+pos.rows,1.0*(pos.pos_x)/C,           1.0*(pos.pos_y+pos.rows)/C, c));
                }
                else
                {
                  vb.push_back(vertex((ii.x)/64.0+pos.left,           (ii.y+32)/64-pos.top,         1.0*(pos.pos_x)/C,           1.0*(pos.pos_y)/C,          c));
                  vb.push_back(vertex((ii.x)/64.0+pos.left+pos.width, (ii.y+32)/64-pos.top,         1.0*(pos.pos_x+pos.width)/C, 1.0*(pos.pos_y)/C,          c));
                  vb.push_back(vertex((ii.x)/64.0+pos.left+pos.width, (ii.y+32)/64-pos.top+pos.rows,1.0*(pos.pos_x+pos.width)/C, 1.0*(pos.pos_y+pos.rows)/C, c));
                  vb.push_back(vertex((ii.x)/64.0+pos.left,           (ii.y+32)/64-pos.top+pos.rows,1.0*(pos.pos_x)/C,           1.0*(pos.pos_y+pos.rows)/C, c));
                }
              }
              break;

            case CommandData_c::CMD_RECT:

              if (ii.blurr == 0)
              {
                auto pos = cache.getRect(640, 640, SUBP_NONE, 0).value();
                Color_c c = gamma.forward(ii.c);
                vb.push_back(vertex((ii.x+32)/64,      (ii.y+32)/64,      1.0*(pos.pos_x+5)/C,           1.0*(pos.pos_y+5)/C,          c));
                vb.push_back(vertex((ii.x+32+ii.w)/64, (ii.y+32)/64,      1.0*(pos.pos_x+pos.width-5)/C, 1.0*(pos.pos_y+5)/C,          c));
                vb.push_back(vertex((ii.x+32+ii.w)/64, (ii.y+32+ii.h)/64, 1.0*(pos.pos_x+pos.width-5)/C, 1.0*(pos.pos_y+pos.rows-5)/C, c));
                vb.push_back(vertex((ii.x+32)/64,      (ii.y+32+ii.h)/64, 1.0*(pos.pos_x+5)/C,           1.0*(pos.pos_y+pos.rows-5)/C, c));
              }
              else
              {
                auto pos = cache.getRect(ii.w, ii.h, sp, ii.blurr).value();
                Color_c c = gamma.forward(ii.c);
                vb.push_back(vertex((ii.x+32)/64+pos.left,           (ii.y+32)/64-pos.top,         1.0*(pos.pos_x)/C,           1.0*(pos.pos_y)/C,          c));
                vb.push_back(vertex((ii.x+32)/64+pos.left+pos.width, (ii.y+32)/64-pos.top,         1.0*(pos.pos_x+pos.width)/C, 1.0*(pos.pos_y)/C,          c));
                vb.push_back(vertex((ii.x+32)/64+pos.left+pos.width, (ii.y+32)/64-pos.top+pos.rows,1.0*(pos.pos_x+pos.width)/C, 1.0*(pos.pos_y+pos.rows)/C, c));
                vb.push_back(vertex((ii.x+32)/64+pos.left,           (ii.y+32)/64-pos.top+pos.rows,1.0*(pos.pos_x)/C,           1.0*(pos.pos_y+pos.rows)/C, c));
              }
              break;

            case CommandData_c::CMD_IMAGE:
              if (images)
                images->draw(ii.x+sx, ii.y+sy, ii.w, ii.h, ii.imageURL);
              break;
          }
          k++;
        }

        if (dc && !cleared && j == dat.size())
        {
          if (dc->vBuffer == 0) { glGenBuffers(1, &dc->vBuffer); } glBindBuffer(GL_ARRAY_BUFFER, dc->vBuffer);
          glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*vb.size(), vb.data(), GL_STATIC_DRAW);

          drawBuffers(sp, vb.size(), sx, sy);

          dc->atlasId = atlasId;
          dc->elements = vb.size();

          return;
        }
        else
        {
          glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
          glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*vb.size(), vb.data(), GL_STREAM_DRAW);

          drawBuffers(sp, vb.size(), sx, sy);
        }

        i = j;

        if (i < dat.size())
        {
          // atlas is not big enough, it needs to be cleared
          // and will be repopulated for the next batch of the layout
          cache.clear();
          atlasId++;
          cleared = true;
        }
      }

      return;
    }

    /** \brief helper function to setup the projection matrices for
     * the showLayout function. It will change the viewport and the
     * modelview and projection matrix to an orthogonal projection
     */
    void setupMatrixes(int width, int height)
    {
      glViewport(0, 0, width, height);
      program.setUniform("width", width/2.0f);
      program.setUniform("height", height/2.0f);
    }

    /** \brief get a pointer to the texture atlas with all the glyphs
     *
     * This is mainly helpful to check how full the texture atlas is
     */
    const uint8_t * getData(void) const { return cache.getData(); }

    void clear(void)
    {
      cache.clear();
      atlasId++;
    }
};

}

#endif
