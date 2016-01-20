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
#ifndef STLL_TEXTURE_ATLAS_H
#define STLL_TEXTURE_ATLAS_H

#include "rectanglePacker.h"

#include <vector>
#include <cstring>
#include <unordered_map>

namespace STLL { namespace internal {

// a texture atlas, allowing you to store texture
// snippets, K is used to reference the elements
// D is the data stored with each element, the stored texture will
// P additional data sent to the add element function
// use B bytes for each pixel
template <class K, class D, class P, int B>
class TextureAtlas_c
{
  private:

    RectanglePacker_c r;
    std::unordered_map<K, D> map;
    std::vector<uint8_t> data;

    // whenever the content of the texture is changed, this value is updated
    uint32_t version;

  protected:

    template <class... A>
    std::tuple<typename std::unordered_map<K, D>::iterator, bool> insert(const K & key, uint32_t w, uint32_t h, A... args)
    {
      auto p = r.allocate(w, h);

      if (p)
      {
        auto pos = p.value();

        version++;

        return std::make_tuple(map.insert(std::make_pair(key, D(pos[0], pos[1], w, h, args...))).first, true);
      }
      else
      {
        return std::make_tuple(map.end(), false);
      }
    }

  public:

    TextureAtlas_c(uint32_t width, uint32_t height) : r(width, height), data(width*height*B) { }

    // the find can not find the requested element in the atlas, it will call this function
    // to add another element, the function needs to return the iterator to the added element
    // or map.end, if it can not add the element,
    // if you need to know the reasons why it could not be added you have to handle this in the
    // derived class
    // The function should create the image to add then then call insert to include
    // the image, the return of insert should be returned... e.g.
    // addElement(const Key & key) { auto i = loadImage(key.name); return insert(key, i.w, i.h, i.data); }
    virtual typename std::unordered_map<K, D>::iterator addElement(const K & key, const P & a) = 0;

    // get an element from the texture atlas, if the element is not on the atlas the
    // addElement function will be called to add it, when that function can not att
    // the element, the returned optional will be empty, otherwise you will get
    // the data for the element
    std::experimental::optional<D> find(const K & key, const P & a)
    {
      auto i = map.find(key);
      if (i != map.end())
      {
        // element was there
        return i->second;
      }
      else
      {
        i = addElement(key, a);

        if (i != map.end())
        {
          // element was not there but insertion succeeded
          return i->second;
        }
        else
        {
          // element was not there and we could not add it :(
          return std::experimental::optional<D>();
        }
      }
    }

    const uint8_t * getData(void) const { return data.data(); }
    uint8_t * getData(void) { return data.data(); }

    uint32_t width(void) const { return r.width(); }
    uint32_t height(void) const { return r.height(); }

    uint32_t getVersion(void) const { return version; }

    void clear(void) {
      r.clear();
      map.clear();
      std::fill(data.begin(), data.end(), 0);
    }
};

} }

#endif
