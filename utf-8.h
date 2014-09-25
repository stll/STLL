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
#ifndef __UTF_8_H__
#define __UTF_8_H__

/** \file
 *  \brief utf-8 helper code.
 */

#include <string>

namespace STLL {

/** \brief Check, if a given string is a valid utf-8 string
 *
 * \param str the string to Check
 * \return true, when utf-8 conform, false otherwise
 */
bool u8_isValid(const std::string & str);

/** \brief Convert an utf-8 string to utf-32. No checking is done here, if the string
 * might come from an unsafe source, check it first
 *
 * \param in the string to convert
 * \return the utf-32 encoded string
 */
std::u32string u8_convertToU32(const std::string & in);

/** \brief Convert a single unicode character to an utf-8 string
 * \param ch the character to Convert
 * \return the corresponding utf-8 string
 */
std::string U32ToUTF8(char32_t ch);

}

#endif