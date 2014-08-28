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

namespace STLL {


static bool ruleFits(const std::string & sel, const pugi::xml_node & node)
{
  if (sel == node.name()) return true;
  if (sel[0] == '.')
  {
    for (auto attr: node.attributes())
      if ((std::string("class") == attr.name()) && (attr.value() == sel.substr(1)))
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

      if (tag == node.name())
      {
        auto a = node.attribute(attr.c_str());
        if (!a.empty())
        {
          std::string nodeattrval = std::string(a.value());
          if (val.length() <= nodeattrval.length() && nodeattrval.substr(0, val.length()) == val)
            return true;
        }
      }
    }
  }

  return false;
}

static uint16_t rulePrio(const std::string & sel)
{
  if (sel[0] == '.') return 2;
  if (sel.find_first_of('[') != sel.npos) return 2;

  return 1;
}

static bool isInheriting(const std::string & attribute)
{
  if (attribute == "color") return true;
  if (attribute == "font-family") return true;
  if (attribute == "font-style") return true;
  if (attribute == "font-size") return true;
  if (attribute == "font-variant") return true;
  if (attribute == "font-weight") return true;
  if (attribute == "padding") return false;
  if (attribute == "margin") return false;
  if (attribute == "text-align") return true;
  if (attribute == "text-align-last") return true;
  if (attribute == "text-indent") return true;
  if (attribute == "direction") return true;
  if (attribute == "border-width") return false;
  if (attribute == "border-color") return false;
  if (attribute == "background-color") return false;
  if (attribute == "text-decoration") return false;
  if (attribute == "text-shadow") return true;
  if (attribute == "width") return false;
  if (attribute == "border-collapse") return true;

  assert(0);
}

static const std::string & getDefault(const std::string & attribute)
{
  static std::string defaults[]= { "sans", "normal", "0px", "", "ltr", "transparent", "separate" };

  if (attribute == "color") throw XhtmlException_c("You must specify the required colors, there is no default");
  if (attribute == "font-family") return defaults[0];
  if (attribute == "font-style") return defaults[1];
  if (attribute == "font-size") throw XhtmlException_c("You must specify all required font sizes, there is no default");
  if (attribute == "font-variant") return defaults[1];
  if (attribute == "font-weight") return defaults[1];
  if (attribute == "padding") return defaults[2];
  if (attribute == "margin") return defaults[2];
  if (attribute == "text-align") return defaults[3];
  if (attribute == "text-align-last") return defaults[3];
  if (attribute == "text-indent") return defaults[2];
  if (attribute == "direction") return defaults[4];
  if (attribute == "border-width") return defaults[2];
  if (attribute == "border-color") return defaults[3];
  if (attribute == "background-color") return defaults[5];
  if (attribute == "text-decoration") return defaults[3];
  if (attribute == "text-shadow") return defaults[3];
  if (attribute == "width") throw XhtmlException_c("You must specify the width, there is no default");
  if (attribute == "border-collapse") return defaults[6];

  assert(0);
}

static bool isValidAttribute(const std::string & attribute)
{
  if (attribute == "color") return true;
  if (attribute == "font-family") return true;
  if (attribute == "font-style") return true;
  if (attribute == "font-size") return true;
  if (attribute == "font-variant") return true;
  if (attribute == "font-weight") return true;
  if (attribute == "padding") return true;
  if (attribute == "margin") return true;
  if (attribute == "text-align") return true;
  if (attribute == "text-align-last") return true;
  if (attribute == "text-indent") return true;
  if (attribute == "direction") return true;
  if (attribute == "border-width") return true;
  if (attribute == "border-color") return true;
  if (attribute == "background-color") return true;
  if (attribute == "text-decoration") return true;
  if (attribute == "text-shadow") return true;
  if (attribute == "width") return true;
  if (attribute == "border-collapse") return true;

  return false;
}

const std::string & textStyleSheet_c::getValue(pugi::xml_node node, const std::string & attribute) const
{
  // go through all rules, check only the ones that give a value to the requested attribute
  // evaluate rule by priority (look at the CSS priority rules
  // choose the highest priority

  while (!node.empty())
  {
    uint16_t prio = 0;
    size_t bestI;

    for (size_t i = 0; i < rules.size(); i++)
    {
      if (   rules[i].attribute == attribute
          && ruleFits(rules[i].selector, node)
          && rulePrio(rules[i].selector) > prio
         )
      {
        prio = rulePrio(rules[i].selector);
        bestI = i;
      }
    }

    if (prio)
      return rules[bestI].value;

    if (!isInheriting(attribute))
      return getDefault(attribute);

    node = node.parent();
  }

  return getDefault(attribute);
}

void textStyleSheet_c::font(const std::string& family, const fontResource_c & res, const std::string& style, const std::string& variant, const std::string& weight, const std::string& stretch)
{
  auto i = families.find(family);

  if (i == families.end())
  {
    i = families.insert(std::make_pair(family, std::make_shared<fontFamily_c>(cache))).first;
  }

  i->second->addFont(res, style, variant, weight, stretch);
}

void textStyleSheet_c::addRule(const std::string sel, const std::string attr, const std::string val)
{
  if (!isValidAttribute(attr))
    throw XhtmlException_c(std::string("attribute not supported: ") + attr);

  rule r;
  r.selector = sel;
  r.attribute = attr;
  r.value = val;

  rules.push_back(r);
}

textStyleSheet_c::textStyleSheet_c(std::shared_ptr< fontCache_c > c)
{
  if (c)
  {
    cache = c;
  }
  else
  {
    cache = std::make_shared<fontCache_c>();
  }
}

}
