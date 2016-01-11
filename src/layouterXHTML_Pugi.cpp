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

#include "layouterXHTML_internals.h"

namespace STLL {

TextLayout_c layoutXML(pugi::xml_node txt, const TextStyleSheet_c & rules, const Shape_c & shape)
{
  return internal::layoutXML_int(txt, rules, shape);
}


/** \brief layout the given XHTML code
 *  \param txt the html text to parse, is must be utf-8. The text must be a proper XHTML document (see also \ref html_sec)
 *  \param rules the stylesheet to use for layouting
 *  \param shape the shape to layout into
 *  \attention it is not checked that txt is proper utf-8. If you have unsafe sources
 *  for your text to layout, use the check function from the utf-8 module
 */
TextLayout_c layoutXHTMLPugi(const std::string & txt, const TextStyleSheet_c & rules, const Shape_c & shape)
{
  auto res = xml_parseStringPugi(txt);

  if (std::get<1>(res) != "")
  {
    throw XhtmlException_c(std::get<1>(res));
  }

  return layoutXML(STLL::xml_getHeadNode(std::get<0>(res)), rules, shape);
}

};