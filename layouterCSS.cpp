#include "layouterCSS.h"

#include <vector>
#include <string>
#include <memory>
#include <algorithm>

static uint8_t hex2num(char c)
{
  switch (c)
  {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'a':
    case 'A': return 10;
    case 'b':
    case 'B': return 11;
    case 'c':
    case 'C': return 12;
    case 'd':
    case 'D': return 13;
    case 'e':
    case 'E': return 14;
    case 'f':
    case 'F': return 15;
    default:  return 0;
  }
}

static uint8_t hex2byte(char c1, char c2)
{
  return hex2num(c1)* 16 + hex2num(c2);
}

void evalColor(const std::string & col, uint8_t & r, uint8_t & g, uint8_t &b)
{
  if (col[0] == '#' && col.length() >= 7)
  {
    r = hex2byte(col[1], col[2]);
    g = hex2byte(col[3], col[4]);
    b = hex2byte(col[5], col[6]);
  }
}

double evalSize(const std::string & sz)
{
  // right now we accept only pixel sizes
  size_t l = sz.length();

  if (sz[l-2] == 'p' && sz[l-1] == 'x')
  {
    return atof(sz.c_str());
  }

  // TODO exception
  return 0;
}

static bool ruleFits(const std::string & sel, const pugi::xml_node & node)
{
  if (sel == "*")         return true;
  if (sel == node.name()) return true;
  if (sel[0] == '.')
  {
    auto attr = node.attribute("class");

    if (!attr.empty())
    {
      if (attr.value() == sel.substr(1))
      {
        return true;
      }
    }
  }

  return false;
}

static uint16_t rulePrio(const std::string & sel)
{
  if (sel == "*") return 0;
  if (sel[0] == '.') return 1;

  return 2;
}

const std::string & textStyleSheet_c::getValue(const pugi::xml_node & node, const std::string & attribute) const
{
  static std::string defaultValue("");

  // go through all rules, check only the ones that give a value to the requested attribute
  // evaluate rule by priority (look at the CSS priority rules
  // choose the highest priority
  int16_t bestPrio = -1;
  std::string & res = defaultValue;

  for (auto & r : rules)
  {
    if (ruleFits(r.selector, node) && r.attribute == attribute && rulePrio(r.selector) > bestPrio)
    {
      res = r.value;
      bestPrio = rulePrio(r.selector);
    }
  }

  return res;
}

void textStyleSheet_c::font(const std::string& family, const std::string& file, const std::string& style, const std::string& variant, const std::string& weight, const std::string& stretch)
{
  if (families.size())
  {
    font(families.begin()->second->getCache(), family, file, style, variant, weight, stretch);
  }
  else
  {
    font(std::make_shared<fontCache_c>(), family, file, style, variant, weight, stretch);
  }
}

void textStyleSheet_c::font(std::shared_ptr<fontCache_c> fc, const std::string& family, const std::string& file, const std::string& style, const std::string& variant, const std::string& weight, const std::string& stretch)
{
  auto i = families.find(family);

  if (i == families.end())
  {
    i = families.insert(std::make_pair(family, std::make_shared<fontFamily_c>(fc))).first;
  }

  i->second->addFont(file, style, variant, weight, stretch);
}

