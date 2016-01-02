#include "layouterXHTML_internals.h"

namespace STLL {

TextLayout_c layoutXML(const xmlNode * txt, const textStyleSheet_c & rules, const Shape_c & shape)
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
TextLayout_c layoutXHTMLLibXML2(const std::string & txt, const textStyleSheet_c & rules, const Shape_c & shape)
{
  return layoutXML(STLL::xml_getHeadNode(std::get<0>(xml_parseStringLibXML2(txt))), rules, shape);
}

};