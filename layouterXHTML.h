#ifndef __LAYOUTER_XHTML_H__
#define __LAYOUTER_XHTML_H__

/** \file
 *  \brief This module contains the XHTML parser and layouter
 */

#include "layouterCSS.h"
#include "layouter.h"

#include <pugixml.hpp>

#include <string>

#include <stdexcept>

namespace STLL {

/** \brief layout the given XHTML code
 *  \param txt the html text to parse, is must be utf-8
 *  \param rules the stylesheet to use for layouting
 *  \param shape the shape to layout into
 *  \attention it is not checked that txt is proper utf-8. If you have unsafe sources
 *  for your text to layout, use the check function from the utf-8 module
 */
textLayout_c layoutXHTML(const std::string & txt, const textStyleSheet_c & rules, const shape_c & shape);

/** \brief layout the given preparsed XML tree as an HTML dom tree
 *  \param xml the xml tree to layout
 *  \param rules the stylesheet to use for layouting
 *  \param shape the shape to layout into
 */
textLayout_c layoutXML(const pugi::xml_document & xml, const textStyleSheet_c & rules, const shape_c & shape);

}

#endif
