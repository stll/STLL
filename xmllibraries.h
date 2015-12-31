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
#ifndef __XML_LIBRARIES_H__
#define __XML_LIBRARIES_H__

/** \file
 *  \brief this file contains overloaded interface functions for all supported XML libraries
 */


#ifdef USE_PUGI_XML
#include <pugixml.hpp>

inline bool xml_isEmpty(const pugi::xml_node & i) { return i.empty(); }
inline bool xml_isDataNode(const pugi::xml_node & i) { return i.type() == pugi::node_pcdata; }
inline bool xml_isElementNode(const pugi::xml_node & i) { return i.type() == pugi::node_element; }

inline const char * xml_getName(const pugi::xml_node & i) { return i.name(); }
inline pugi::xml_node xml_getParent(const pugi::xml_node & i) { return i.parent(); }
inline pugi::xml_node xml_getFirstChild(const pugi::xml_node & i) { return i.first_child(); }
inline pugi::xml_node xml_getNextSibling(const pugi::xml_node & i) { return i.next_sibling(); }

inline const char * xml_getAttribute(const pugi::xml_node & i, const char * attr) {
  auto a = i.attribute(attr);

  if (a)
    return a.value();
  else
    return 0;
}

template <class F>
bool xml_forEachChild(const pugi::xml_node & i, F f)
{
  for (auto a : i)
  {
    if (f(a))
    {
      return true;
    }
  }
  return false;
}

template <class F>
bool xml_forEachAttribute(const pugi::xml_node & i, F f)
{
  for (auto a : i.attributes())
  {
    if (f(a.name(), a.value()))
    {
      return true;
    }
  }
  return false;
}

#endif




#ifdef USE_LIBXML2
#include <libxml/tree.h>

inline bool xml_isEmpty(const xmlNode * i) { return i == nullptr; }
inline bool xml_isDataNode(const xmlNode * i) { return i->type == XML_TEXT_NODE; }
inline bool xml_isElementNode(const xmlNode * i) { return i->type == XML_ELEMENT_NODE; }

inline const char * xml_getName(const xmlNode * i) { return (const char*)(i->name); }
inline const xmlNode * xml_getParent(const xmlNode * i) { return i->parent; }
inline const xmlNode * xml_getFirstChild(const xmlNode * i) { return i->children; }
inline const xmlNode * xml_getNextSibling(const xmlNode * i) { return i->next; }

inline const char * xml_getAttribute(const xmlNode * i, const char * attr) {
  return (const char*)xmlGetProp(i, (const xmlChar*)attr);
}

template <class F>
bool xml_forEachChild(const xmlNode * i, F f)
{
  for (auto c = i->children; c; c = c->next)
  {
    if (f(c))
    {
      return true;
    }
  }
  return false;
}

template <class F>
bool xml_forEachAttribute(const xmlNode * i, F f) {
  for (auto c = i->properties; c; c = c->next)
  {
    xmlChar *content = xmlNodeListGetString(i->doc, c->children, 1);
    bool b = f((const char*)c->name, (const char *)content);
    xmlFree(content);

    if (b)
    {
      return true;
    }
  }

  return false;
}

#endif



#endif
