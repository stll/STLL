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
#ifndef STLL_LAYOUTER_CSS_INT_H
#define STLL_LAYOUTER_CSS_INT_H

#include "xmllibraries.h"

#include <string>

namespace STLL { namespace internal {

template <class X>
bool ruleFits(const std::string & sel, const X node)
{
  if (sel == xml_getName(node)) return true;
  if (sel[0] == '.')
  {
    if (xml_forEachAttribute(node, [sel](const char * n, const char * v) {
      return (std::string("class") == n) && (v == sel.substr(1)); } ))
    {
      return true;
    }
  }
  if (sel.find_first_of('[') != sel.npos)
  {
    size_t st = sel.find_first_of('[');
    size_t en = sel.find_first_of(']');
    size_t mi = sel.find_first_of('=');

    if (sel[mi-1] == '|')
    {
      std::string tag = sel.substr(0, st);
      std::string attr = sel.substr(st+1, mi-2-st);
      std::string val = sel.substr(mi+1, en-mi-1);

      if (tag == xml_getName(node))
      {
        const char * a = xml_getAttribute(node, attr.c_str());
        if (a)
        {
          std::string nodeattrval = std::string(a);
          if (val.length() <= nodeattrval.length() && nodeattrval.substr(0, val.length()) == val)
            return true;
        }
      }
    }
  }

  return false;
}

uint16_t rulePrio(const std::string & sel);
bool isInheriting(const std::string & attribute);
const std::string & getDefault(const std::string & attribute);

} }



#endif
