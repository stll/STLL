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
#ifndef __LAYOUTER_SDL__
#define __LAYOUTER_SDL__

/** \file
 *  \brief SDL output driver
 */

#ifdef HAVE_SDL

#include "layouter.h"

#include <SDL.h>

#include <vector>

namespace STLL {

/** \brief display a single layout
 *  \param l layout to draw
 *  \param sx x position on the target surface
 *  \param sy y position on the target surface
 *  \param s target surface
 */
void showLayoutSDL(const textLayout_c & l, int sx, int sy, SDL_Surface * s);

}

#endif

#endif
