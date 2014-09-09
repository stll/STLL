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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef __COLOR_H__
#define __COLOR_H__

#include <stdint.h>

/** \file
 *  \brief this file contains just a little helper class for a 4-value color
 */

namespace STLL {

class color_c
{
  private:
    uint8_t val[4];

  public:

    color_c(void) : val{0, 0, 0, 0} {}
    color_c(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : val{r, g, b, a} {}

    uint8_t r(void) const { return val[0]; }
    uint8_t g(void) const { return val[1]; }
    uint8_t b(void) const { return val[2]; }
    uint8_t a(void) const { return val[3]; }

    bool operator== (const color_c & rhs) const
    {
      return (val[0] == rhs.val[0]) && (val[1] == rhs.val[1]) &&
             (val[2] == rhs.val[2]) && (val[3] == rhs.val[3]);
    }

    void operator= (const color_c & rhs)
    {
      val[0] = rhs.val[0];
      val[1] = rhs.val[1];
      val[2] = rhs.val[2];
      val[3] = rhs.val[3];
    }
};

}

#endif
