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

#ifndef __BLURR_H__
#define __BLURR_H__

/** \file
 *  \brief a gaussian blurr
 */

#include <cstdint>

namespace STLL { namespace internal {

/** \brief apply a gaussian blurr to a 1 channel image
 *
 * \param s the byte array to apply the blurr to, it is assumed to point to w*h bytes
 * \param pitch number of bytes to get to the next line
 * \param w the width of the image
 * \param h the hight of the data
 * \param r the radius to spread the data over
 * \param sx scaling factor for the x direction, using a value bigger than 1 will increase the blurr radius accordingly
 * \param sy scaling factor for the y direction, using a value bigger than 1 will increase the blurr radius accordingly
 */
void gaussBlur (uint8_t * s, int pitch, int w, int h, double r, int sx, int sy);

/** \brief calculates how far information can spread when applying this blurr
 */
int gaussBlurrDist(double r);

} }


#endif
