#ifndef __LAYOUTER_XHTML_H__
#define __LAYOUTER_XHTML_H__

#include "layouterCSS.h"
#include "layouter.h"

#include <pugixml.hpp>

#include <string>

// layout the given XHTML code
textLayout_c layoutXHTML(const std::string & txt, const textStyleSheet_c & rules, const shape_c & shape);

// layout the given pugi-XML nodes, they must be a parsed XHTML document
textLayout_c layoutXML(const pugi::xml_document & txt, const textStyleSheet_c & rules, const shape_c & shape);

#endif
