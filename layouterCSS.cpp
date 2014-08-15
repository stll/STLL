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
    default:
      throw XhtmlException_c("Wrong format for a hex-number");
  }
}

static uint8_t hex2byte(char c1, char c2)
{
  return hex2num(c1)* 16 + hex2num(c2);
}

void evalColor(const std::string & col, uint8_t & r, uint8_t & g, uint8_t &b, uint8_t &a)
{
  if (col == "transparent")
  {
    r = g = b = a = 0;
    return;
  }

  if (col.length() != 7)
    throw XhtmlException_c("color string must be 7 characters long");

  if (col[0] == '#')
  {
    r = hex2byte(col[1], col[2]);
    g = hex2byte(col[3], col[4]);
    b = hex2byte(col[5], col[6]);
    a = 255;
  }
  else
    throw XhtmlException_c("only the # color scheme is supported and keyword transparent");
}

double evalSize(const std::string & sz)
{
  // right now we accept only pixel sizes
  size_t l = sz.length();

  if (sz[l-2] == 'p' && sz[l-1] == 'x')
  {
    return atof(sz.c_str());
  }

  throw XhtmlException_c("only pixel size format is supported");

  return 0;
}

static bool ruleFits(const std::string & sel, const pugi::xml_node & node)
{
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

  assert(0);
}

static const std::string getDefault(const std::string & attribute)
{
  if (attribute == "color") throw XhtmlException_c("You must specify the required colors, there is no default");
  if (attribute == "font-family") return "sans";
  if (attribute == "font-style") return "normal";
  if (attribute == "font-size") throw XhtmlException_c("You must specify all required font sizes, there is no default");
  if (attribute == "font-variant") return "normal";
  if (attribute == "font-weight") return "normal";
  if (attribute == "padding") return "0px";
  if (attribute == "margin") return "0px";
  if (attribute == "text-align") return "";
  if (attribute == "text-align-last") return "";
  if (attribute == "text-indent") return "0px";
  if (attribute == "direction") return "ltr";
  if (attribute == "border-width") return "0px";
  if (attribute == "border-color") return "";
  if (attribute == "background-color") return "transparent";
  if (attribute == "text-decoration") return "";

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

  return false;
}

const std::string textStyleSheet_c::getValue(pugi::xml_node node, const std::string & attribute) const
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

