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
#ifndef STLL_XML_LIBRARIES_H
#define STLL_XML_LIBRARIES_H

#include <tuple>

/** \file
 *  \brief this file contains overloaded interface functions for all supported XML libraries
 */


#ifdef USE_PUGI_XML
#include <pugixml.hpp>
#include <boost/lexical_cast.hpp>
#include <memory>

namespace STLL { namespace internal {

inline std::tuple<std::unique_ptr<pugi::xml_document>, std::string> xml_parseStringPugi(const std::string & txt)
{
  auto doc = std::make_unique<pugi::xml_document>();
  std::string error;

  auto res = doc->load_buffer(txt.c_str(), txt.length(), pugi::parse_ws_pcdata);

  if (!res)
  {
    error =  std::string("Error Parsing XHTML [") + doc->child("node").attribute("attr").value() + "]\n" +
             "Error description: " + res.description() + "\n" +
             "Error offset: " + boost::lexical_cast<std::string>(res.offset) + "  " +
             txt.substr(std::max<int>(res.offset-20, 0), 20) + "[here]" + txt.substr(res.offset, 20);
  }

  return std::make_tuple(std::move(doc), std::move(error));
}

inline pugi::xml_node xml_getHeadNode(const std::unique_ptr<pugi::xml_document> & doc)
{
  return doc->document_element();
}

inline bool xml_isEmpty(pugi::xml_node i) { return i.empty(); }
inline bool xml_isDataNode(pugi::xml_node i) { return i.type() == pugi::node_pcdata; }
inline bool xml_isElementNode(pugi::xml_node i) { return i.type() == pugi::node_element; }

inline const char * xml_getName(pugi::xml_node i) { return i.name(); }
inline const char * xml_getData(pugi::xml_node i) { return i.value(); }
inline pugi::xml_node xml_getParent(pugi::xml_node i) { return i.parent(); }
inline pugi::xml_node xml_getFirstChild(pugi::xml_node i) { return i.first_child(); }
inline pugi::xml_node xml_getNextSibling(pugi::xml_node i) { return i.next_sibling(); }
inline pugi::xml_node xml_getPreviousSibling(pugi::xml_node i) { return i.previous_sibling(); }

inline const char * xml_getAttribute(pugi::xml_node i, const char * attr) {
  auto a = i.attribute(attr);

  if (a)
    return a.value();
  else
    return 0;
}

template <class F>
bool xml_forEachChild(pugi::xml_node i, F f)
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
bool xml_forEachAttribute(pugi::xml_node i, F f)
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

} }

#endif




#ifdef USE_LIBXML2
#include <libxml/tree.h>

namespace STLL { namespace internal {

class libxml2Doc_c {
  public:
    xmlDoc * doc;

    libxml2Doc_c(xmlDoc * d) : doc(d) {}
    ~libxml2Doc_c(void)
    {
      if (doc)
        xmlFreeDoc(doc);
    }
    libxml2Doc_c(libxml2Doc_c && d) : doc(d.doc) { d.doc = 0; }
};

inline std::tuple<libxml2Doc_c, std::string> xml_parseStringLibXML2(const std::string & txt)
{
  LIBXML_TEST_VERSION

  /*parse the file and get the DOM */
  libxml2Doc_c doc(xmlReadDoc((const xmlChar*)txt.c_str(), "", "utf-8", XML_PARSE_NOERROR + XML_PARSE_NOWARNING));
  std::string error;

  if ((doc.doc == nullptr) || !(doc.doc->properties & XML_DOC_WELLFORMED))
  {
    // TODO correct error information
    error = std::string("Error Parsing XHTML [");
  }

  return std::make_tuple(std::move(doc), std::move(error));
}

inline const xmlNode * xml_getHeadNode(const libxml2Doc_c & doc)
{
  return xmlDocGetRootElement(doc.doc);
}

inline bool xml_isEmpty(const xmlNode * i) { return i == nullptr; }
inline bool xml_isDataNode(const xmlNode * i) { return i->type == XML_TEXT_NODE; }
inline bool xml_isElementNode(const xmlNode * i) { return i->type == XML_ELEMENT_NODE; }

inline const char * xml_getName(const xmlNode * i) { if (i && i->name) return (const char*)(i->name); else return ""; }
inline const char * xml_getData(const xmlNode * i) { if (i && i->content) return (const char*)(i->content); else return ""; }
inline const xmlNode * xml_getParent(const xmlNode * i) { return i->parent; }
inline const xmlNode * xml_getFirstChild(const xmlNode * i) { return i->children; }
inline const xmlNode * xml_getNextSibling(const xmlNode * i) { return i->next; }
inline const xmlNode * xml_getPreviousSibling(const xmlNode * i) { return i->prev; }

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

} }

#endif

#endif
