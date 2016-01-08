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
#include "layouterCSS.h"

#include <vector>
#include <string>
#include <memory>
#include <algorithm>

#include <assert.h>

namespace STLL {

namespace internal {

uint16_t rulePrio(const std::string & sel)
{
  if (sel[0] == '.') return 2;
  if (sel.find_first_of('[') != sel.npos) return 2;

  return 1;
}

bool isInheriting(const std::string & attribute)
{
  if (attribute == "color") return true;
  if (attribute == "font-family") return true;
  if (attribute == "font-style") return true;
  if (attribute == "font-size") return true;
  if (attribute == "font-variant") return true;
  if (attribute == "font-weight") return true;
//  if (attribute == "padding") return false;
//  if (attribute == "padding-left") return false;
//  if (attribute == "padding-right") return false;
//  if (attribute == "padding-top") return false;
//  if (attribute == "padding-bottom") return false;
//  if (attribute == "margin") return false;
//  if (attribute == "margin-left") return false;
//  if (attribute == "margin-right") return false;
//  if (attribute == "margin-top") return false;
//  if (attribute == "margin-bottom") return false;
  if (attribute == "text-align") return true;
  if (attribute == "text-align-last") return true;
  if (attribute == "text-indent") return true;
  if (attribute == "direction") return true;
//  if (attribute == "border-top-width") return false;
//  if (attribute == "border-bottom-width") return false;
//  if (attribute == "border-left-width") return false;
//  if (attribute == "border-right-width") return false;
//  if (attribute == "border-width") return false;
//  if (attribute == "border-left-color") return false;
//  if (attribute == "border-right-color") return false;
//  if (attribute == "border-top-color") return false;
//  if (attribute == "border-bottom-color") return false;
//  if (attribute == "border-color") return false;
//  if (attribute == "background-color") return false;
  if (attribute == "text-decoration") return true;
  if (attribute == "text-shadow") return true;
//  if (attribute == "width") return false;
  if (attribute == "border-collapse") return true;
//  if (attribute == "vertical-align") return false;

  return false;
}

const std::string & getDefault(const std::string & attribute)
{
  static std::string defaults[]= { "sans", "normal", "0px", "", "ltr", "transparent", "separate", "baseline" };

  if (attribute == "color") throw XhtmlException_c("You must specify the required colors, there is no default");
  if (attribute == "font-family") return defaults[0];
  if (attribute == "font-style") return defaults[1];
  if (attribute == "font-size") throw XhtmlException_c("You must specify all required font sizes, there is no default");
  if (attribute == "font-variant") return defaults[1];
  if (attribute == "font-weight") return defaults[1];
  if (attribute == "padding") return defaults[2];
//  if (attribute == "padding-left") return defaults[3];
//  if (attribute == "padding-right") return defaults[3];
//  if (attribute == "padding-top") return defaults[3];
//  if (attribute == "padding-bottom") return defaults[3];
  if (attribute == "margin") return defaults[2];
//  if (attribute == "margin-left") return defaults[3];
//  if (attribute == "margin-right") return defaults[3];
//  if (attribute == "margin-top") return defaults[3];
//  if (attribute == "margin-bottom") return defaults[3];
//  if (attribute == "text-align") return defaults[3];
//  if (attribute == "text-align-last") return defaults[3];
  if (attribute == "text-indent") return defaults[2];
  if (attribute == "direction") return defaults[4];
  if (attribute == "border-width") return defaults[2];
//  if (attribute == "border-left-width") return defaults[3];
//  if (attribute == "border-right-width") return defaults[3];
//  if (attribute == "border-top-width") return defaults[3];
//  if (attribute == "border-bottom-width") return defaults[3];
//  if (attribute == "border-color") return defaults[3];
//  if (attribute == "border-top-color") return defaults[3];
//  if (attribute == "border-bottom-color") return defaults[3];
//  if (attribute == "border-left-color") return defaults[3];
//  if (attribute == "border-right-color") return defaults[3];
  if (attribute == "background-color") return defaults[5];
//  if (attribute == "text-decoration") return defaults[3];
//  if (attribute == "text-shadow") return defaults[3];
  if (attribute == "width") throw XhtmlException_c("You must specify the width, there is no default");
  if (attribute == "border-collapse") return defaults[6];
  if (attribute == "vertical-align") return defaults[7];

  return defaults[3];
}

};

static bool isValidAttribute(const std::string & attribute)
{
  if (attribute == "color") return true;
  if (attribute == "font-family") return true;
  if (attribute == "font-style") return true;
  if (attribute == "font-size") return true;
  if (attribute == "font-variant") return true;
  if (attribute == "font-weight") return true;
  if (attribute == "padding") return true;
  if (attribute == "padding-left") return true;
  if (attribute == "padding-right") return true;
  if (attribute == "padding-top") return true;
  if (attribute == "padding-bottom") return true;
  if (attribute == "margin") return true;
  if (attribute == "margin-left") return true;
  if (attribute == "margin-right") return true;
  if (attribute == "margin-top") return true;
  if (attribute == "margin-bottom") return true;
  if (attribute == "text-align") return true;
  if (attribute == "text-align-last") return true;
  if (attribute == "text-indent") return true;
  if (attribute == "direction") return true;
  if (attribute == "border-width") return true;
  if (attribute == "border-left-width") return true;
  if (attribute == "border-right-width") return true;
  if (attribute == "border-top-width") return true;
  if (attribute == "border-bottom-width") return true;
  if (attribute == "border-color") return true;
  if (attribute == "border-left-color") return true;
  if (attribute == "border-right-color") return true;
  if (attribute == "border-top-color") return true;
  if (attribute == "border-bottom-color") return true;
  if (attribute == "background-color") return true;
  if (attribute == "text-decoration") return true;
  if (attribute == "text-shadow") return true;
  if (attribute == "width") return true;
  if (attribute == "border-collapse") return true;
  if (attribute == "vertical-align") return true;

  return false;
}

static bool isHexChar(char c)
{
  switch(c)
  {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
      return true;
    default:
      return false;
  }
}

static bool isNumChar(char c)
{
  switch(c)
  {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return true;
    default:
      return false;
  }
}

static void checkFormatColor(const std::string & value)
{
  if (value[0] == '#')
  {
    if (value.size() != 7)
      throw XhtmlException_c(std::string("wrong length of #-colour value"));

    for (size_t i = 1; i < 7; i++)
      if (!isHexChar(value[i]))
        throw XhtmlException_c(std::string("wrong character in #-colour value ") + value);
  }
  else if (value == "transparent")
  {
    // ok
  }
  else
    throw XhtmlException_c(std::string("only #-colour syntax or the keyword 'transparent' is supported for colour"));

}

#define SZ_PX 1
#define SZ_PERCENT 2
#define SZ_RELATIVE 4

static void checkFormatSize(const std::string & value, uint8_t formats)
{
  size_t l = value.length();
  if ((formats & SZ_PX) && value[l-2] == 'p' && value[l-1] == 'x')
  {
    for (size_t i = 0; i < l-2; i++)
      if (!isNumChar(value[i]))
        throw XhtmlException_c(std::string("size format for pixel size not correct ") + value);
  }
  else if ((formats & SZ_PERCENT) && value[l-1] == '%')
  {
    for (size_t i = 0; i < l-1; i++)
      if (!isNumChar(value[i]))
        throw XhtmlException_c(std::string("size format for percent size not correct ") + value);
  }
  else if ((formats & SZ_RELATIVE) && value[l-1] == '*')
  {
    for (size_t i = 0; i < l-1; i++)
      if (!isNumChar(value[i]))
        throw XhtmlException_c(std::string("size format for relative size not correct ") + value);
  }
  else
  {
    throw XhtmlException_c(std::string("size value not pixel or percent format ") + value);
  }
}

static void checkValues(const std::string & value, std::vector<std::string> vals, const std::string & attrib)
{
  if (std::find(vals.begin(), vals.end(), value) == vals.end())
    throw XhtmlException_c(std::string("attribute ") + attrib + " has none of the alloved values " + value);
}

static void checkShadowFormat(const std::string & value)
{
  uint8_t state = 0;

  for (size_t i = 0; i < value.size(); i++)
  {
    switch (value[i])
    {
      case '-':
        if      (state == 0)                 state = 1;
        else if (state == 5)                 state = 6;
        else if (state == 19)                state = 20;
        else                                 state = 0xff;        break;
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        if      (state == 0 || state == 1 || state == 2)   state = 2;
        else if (state == 5 || state == 6 || state == 7)   state = 7;
        else if (state == 19 || state == 20 || state == 21)state = 21;
        else if (state >= 11 && state <= 16) state++;
        else                                 state = 0xff;        break;
      case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        if      (state >= 11 && state <= 16)  state++;
        else                                 state = 0xff;        break;
      case ' ':
        if      (state == 0)                 state = 0;
        else if (state == 4 || state == 5)   state = 5;
        else if (state == 9 || state == 10)  state = 10;
        else if (state == 18 || state == 19) state = 19;
        else if (state == 15)                state = 15;
        else if (state == 16)                state = 16;
        else                                 state = 0xff;        break;
      case 'p':
        if      (state == 2)                 state = 3;
        else if (state == 7)                 state = 8;
        else if (state == 21)                state = 22;
        else                                 state = 0xff;        break;
      case 'x':
        if      (state == 3)                 state = 4;
        else if (state == 8)                 state = 18;
        else if (state == 22)                state = 10;
        else                                 state = 0xff;        break;
      case '#':
        if      (state == 10)                state = 11;
        else                                 state = 0xff;        break;
      case ',':
        if      (state == 17)                state = 0;
        else                                 state = 0xff;        break;
      default:                               state = 0xff;        break;
    }

    if (state == 0xff)
      throw XhtmlException_c(std::string("format for shadow string not correct ") + value);
  }

  if (value != "" && state != 17)
    throw XhtmlException_c(std::string("format for shadow string not correct: no proper end ") + value);
}

static void checkValueFormat(const std::string & attribute, const std::string & value)
{
  if (attribute == "color") checkFormatColor(value);
  if (attribute == "font-family") {} // no restrictions
  if (attribute == "font-style") {} // no restrictions
  if (attribute == "font-size") checkFormatSize(value, SZ_PX + SZ_PERCENT);
  if (attribute == "font-variant") {} // no restrictions
  if (attribute == "font-weight") {} // no restrictions
  if (attribute == "padding") checkFormatSize(value, SZ_PX);
  if (attribute == "padding-left") checkFormatSize(value, SZ_PX);
  if (attribute == "padding-right") checkFormatSize(value, SZ_PX);
  if (attribute == "padding-top") checkFormatSize(value, SZ_PX);
  if (attribute == "padding-bottom") checkFormatSize(value, SZ_PX);
  if (attribute == "margin") checkFormatSize(value, SZ_PX);
  if (attribute == "margin-left") checkFormatSize(value, SZ_PX);
  if (attribute == "margin-right") checkFormatSize(value, SZ_PX);
  if (attribute == "margin-top") checkFormatSize(value, SZ_PX);
  if (attribute == "margin-bottom") checkFormatSize(value, SZ_PX);
  if (attribute == "text-align") checkValues(value, {"left", "right", "center", "justify", ""}, "text-align");
  if (attribute == "text-align-last") checkValues(value, {"left", "right", ""}, "text-align-last");
  if (attribute == "text-indent") checkFormatSize(value, SZ_PX);
  if (attribute == "direction") checkValues(value, {"ltr", "rtl"}, "direction");
  if (attribute == "border-width") checkFormatSize(value, SZ_PX);
  if (attribute == "border-left-width") checkFormatSize(value, SZ_PX);
  if (attribute == "border-right-width") checkFormatSize(value, SZ_PX);
  if (attribute == "border-top-width") checkFormatSize(value, SZ_PX);
  if (attribute == "border-bottom-width") checkFormatSize(value, SZ_PX);
  if (attribute == "border-color") checkFormatColor(value);
  if (attribute == "border-left-color") checkFormatColor(value);
  if (attribute == "border-right-color") checkFormatColor(value);
  if (attribute == "border-top-color") checkFormatColor(value);
  if (attribute == "border-bottom-color") checkFormatColor(value);
  if (attribute == "background-color") checkFormatColor(value);
  if (attribute == "text-decoration") checkValues(value, {"underline", ""}, "text-decoration");
  if (attribute == "text-shadow") checkShadowFormat(value);

  // TODO width in different environments may allow different formats...
  if (attribute == "width") checkFormatSize(value, SZ_PX + SZ_PERCENT + SZ_RELATIVE);
  if (attribute == "border-collapse") checkValues(value, {"collapse", "separate"}, "border-collapse");
  if (attribute == "vertical-align") checkValues(value, {"baseline", "top", "middle", "bottom"}, "vertical-align");
}

void textStyleSheet_c::font(const std::string& family, const FontResource_c & res, const std::string& style, const std::string& variant, const std::string& weight, const std::string& stretch)
{
  auto i = families.find(family);

  if (i == families.end())
  {
    i = families.insert(std::make_pair(family, std::make_shared<FontFamily_c>(cache))).first;
  }

  i->second->addFont(res, style, variant, weight, stretch);
}

static void checkSelectorValidity(const std::string & sel)
{
  // valid attributes to select on
  static std::vector<std::string> val_att { "lang" };
  static std::vector<std::string> val_tag {
    "p", "html", "body", "ul", "li", "img", "table", "th", "tr", "td",
    "h1", "h2", "h3", "h4", "h5", "h6", "sub", "sup", "i", "p", "span", "a"
  };

  if (sel[0] == '.')
  {
    // class selector..
    // TODO check valid format of class name
  }
  else if (sel.find_first_of('[') != sel.npos)
  {
    size_t st = sel.find_first_of('[');
    size_t en = sel.find_first_of(']');
    size_t mi = sel.find_first_of('=');

    if (en == sel.npos || mi == sel.npos)
      throw XhtmlException_c(std::string("attribute selector on attribute with wrong syntax ") + sel);

    if (sel[mi-1] == '|')
    {
      std::string tag = sel.substr(0, st);
      std::string att = sel.substr(st+1, mi-2-st);
      std::string val = sel.substr(mi+1, en-mi-1);

      if (std::find(val_att.begin(), val_att.end(), att) == val_att.end())
        throw XhtmlException_c(std::string("attribute selector on invalid attribute ") + att);

      if (std::find(val_tag.begin(), val_tag.end(), tag) == val_tag.end())
        throw XhtmlException_c(std::string("attribute selector on invalid tag ") + tag);
    }
    else
    {
      throw XhtmlException_c(std::string("attribute selector selector only with |= syntax supported"));
    }
  }
  else
  {
    // selection on tags
    if (std::find(val_tag.begin(), val_tag.end(), sel) == val_tag.end())
      throw XhtmlException_c(std::string("attribute selector on invalid tag ") + sel);
  }
}

void textStyleSheet_c::addRule(const std::string sel, const std::string attr, const std::string val)
{
  checkSelectorValidity(sel);

  if (!isValidAttribute(attr))
    throw XhtmlException_c(std::string("attribute not supported: ") + attr);

  checkValueFormat(attr, val);

  // check, if a rule already exists, and if so, just change the value
  for (auto & a : rules)
    if (a.selector == sel && a.attribute == attr)
    {
      a.value = val;
      return;
    }

  rule r;
  r.selector = sel;
  r.attribute = attr;
  r.value = val;

  rules.push_back(r);
}

textStyleSheet_c::textStyleSheet_c(std::shared_ptr< FontCache_c > c)
{
  if (c)
  {
    cache = c;
  }
  else
  {
    cache = std::make_shared<FontCache_c>();
  }
}

}
