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
#ifndef STLL_HYPHENDICTIONARY_H
#define STLL_HYPHENDICTIONARY_H

#include <cstddef>
#include <string>
#include <istream>
#include <vector>

/** \file
 *  \brief registering of hyphen dictionaries
 */

namespace STLL {

/** \brief register a hyphen dictionary for a given set of languages
 *
 * \param langs a vector of language strings. The language strings are the
 *              ones that you use in the lang xml attributes or in the lang
 *              member of the AttributeIndex class. If the language to be looked
 *              for when looking for a dictionary to do the hyphenation contains
 *              an dash stll will also look for a dictionary with the text
 *              just before that dash. So if you register a dictionary
 *              for the language "en" and use "en-US" in your language tag
 *              it will use your hyphenation dictionary
 * \param str must point to an input stream of a hyphen dictionary, the file
 *            must be an UTF-8 encoded Open Office hyphen dictionary, nothing
 *            else is not supported
 */
void addHyphenDictionary(const std::vector<std::string> & langs, std::istream & str);

/** \brief see the other addHyphenDictionary function
 */
void addHyphenDictionary(const std::vector<std::string> & langs, std::istream && str);

}

#endif