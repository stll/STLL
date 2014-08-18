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

void textStyleSheet_c::font(const std::string& family, const fontRessource_c & res, const std::string& style, const std::string& variant, const std::string& weight, const std::string& stretch)
{
  if (families.size())
  {
    font(families.begin()->second->getCache(), family, std::move(res), style, variant, weight, stretch);
  }
  else
  {
    font(std::make_shared<fontCache_c>(), family, std::move(res), style, variant, weight, stretch);
  }
}

void textStyleSheet_c::font(std::shared_ptr<fontCache_c> fc, const std::string& family, const fontRessource_c & res, const std::string& style, const std::string& variant, const std::string& weight, const std::string& stretch)
{
  auto i = families.find(family);

  if (i == families.end())
  {
    i = families.insert(std::make_pair(family, std::make_shared<fontFamily_c>(fc))).first;
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

}
