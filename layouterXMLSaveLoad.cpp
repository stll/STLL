
#include "layouterXMLSaveLoad.h"

namespace STLL
{

void saveLayoutToXML(const TextLayout_c & l, pugi::xml_node & node, std::shared_ptr<fontCache_c> c)
{
  auto doc = node.append_child();
  doc.set_name("layout");
  doc.append_attribute("height").set_value(l.getHeight());
  doc.append_attribute("left").set_value(l.getLeft());
  doc.append_attribute("right").set_value(l.getRight());

  // output fonts;
  auto fonts = doc.append_child();
  fonts.set_name("fonts");

  std::vector<std::shared_ptr<STLL::fontFace_c>> found;

  for (const auto & a : l.getData())
  {
    if (  (a.command == TextLayout_c::CommandData_c::CMD_GLYPH)
        &&(std::find(found.begin(), found.end(), a.font) == found.end())
       )
    {
      found.push_back(a.font);

      if (!c->containsFont(a.font))
        throw SaveLoadException_c("Font not found in cache, maybe wrong cache?");

      auto fnt = fonts.append_child();
      fnt.set_name("font");
      fnt.append_attribute("file").set_value(c->getFontResource(a.font).getDescription().c_str());
      fnt.append_attribute("size").set_value(c->getFontSize(a.font));
    }
  }

  // output commands
  auto commands = doc.append_child();
  commands.set_name("commands");

  for (const auto & a : l.getData())
  {
    switch (a.command)
    {
      case STLL::TextLayout_c::CommandData_c::CMD_GLYPH:
        {
          auto n = commands.append_child();
          n.set_name("glyph");
          n.append_attribute("x").set_value(a.x);
          n.append_attribute("y").set_value(a.y);
          n.append_attribute("glyphIndex").set_value(static_cast<unsigned int>(a.glyphIndex));
          n.append_attribute("font").set_value(static_cast<int>(std::distance(found.begin(), std::find(found.begin(), found.end(), a.font))));
          n.append_attribute("r").set_value(a.c.r());
          n.append_attribute("g").set_value(a.c.g());
          n.append_attribute("b").set_value(a.c.b());
          n.append_attribute("a").set_value(a.c.a());
        }
        break;
      case STLL::TextLayout_c::CommandData_c::CMD_RECT:
        {
          auto n = commands.append_child();
          n.set_name("rect");
          n.append_attribute("x").set_value(a.x);
          n.append_attribute("y").set_value(a.y);
          n.append_attribute("w").set_value(a.w);
          n.append_attribute("h").set_value(a.h);
          n.append_attribute("r").set_value(a.c.r());
          n.append_attribute("g").set_value(a.c.g());
          n.append_attribute("b").set_value(a.c.b());
          n.append_attribute("a").set_value(a.c.a());
        }
        break;
      case STLL::TextLayout_c::CommandData_c::CMD_IMAGE:
        {
          auto n = commands.append_child();
          n.set_name("image");
          n.append_attribute("x").set_value(a.x);
          n.append_attribute("y").set_value(a.y);
          n.append_attribute("w").set_value(a.w);
          n.append_attribute("h").set_value(a.h);
          n.append_attribute("url").set_value(a.imageURL.c_str());
        }
        break;
    }
  }

  // output links
  if (!l.links.empty())
  {
    auto links = doc.append_child();
    links.set_name("links");

    for (const auto & l2 : l.links)
    {
      auto link = links.append_child();
      link.set_name("link");
      link.append_attribute("url").set_value(l2.url.c_str());

      for (const auto & a : l2.areas)
      {
        auto area = link.append_child();
        area.set_name("area");
        area.append_attribute("x").set_value(a.x);
        area.append_attribute("y").set_value(a.y);
        area.append_attribute("w").set_value(a.w);
        area.append_attribute("h").set_value(a.h);
      }
    }
  }
}


TextLayout_c loadLayoutFromXML(const pugi::xml_node & doc, std::shared_ptr<fontCache_c> c)
{
  // get the fonts from the file
  auto fonts = doc.child("fonts");
  std::vector<std::shared_ptr<fontFace_c>> found;

  for (const auto a : fonts.children())
    found.push_back(c->getFont(fontResource_c(a.attribute("file").value()), std::stoi(a.attribute("size").value())));

  auto commands = doc.child("commands");

  TextLayout_c l;

  for (const auto a : commands.children())
  {
    if (a.name() == std::string("glyph"))
    {
      TextLayout_c::CommandData_c c(
        found[std::stoi(a.attribute("font").value())],
        std::stoi(a.attribute("glyphIndex").value()),
        std::stoi(a.attribute("x").value()),
        std::stoi(a.attribute("y").value()),
        color_c(std::stoi(a.attribute("r").value()), std::stoi(a.attribute("g").value()),
                std::stoi(a.attribute("b").value()), std::stoi(a.attribute("a").value()))
      );

      l.addCommand(c);
    }
    else if (a.name() == std::string("rect"))
    {
      TextLayout_c::CommandData_c c(
        std::stoi(a.attribute("x").value()),
        std::stoi(a.attribute("y").value()),
        std::stoi(a.attribute("w").value()),
        std::stoi(a.attribute("h").value()),
        color_c(std::stoi(a.attribute("r").value()), std::stoi(a.attribute("g").value()),
                std::stoi(a.attribute("b").value()), std::stoi(a.attribute("a").value()))
      );

      l.addCommand(c);
    }
    else if (a.name() == std::string("image"))
    {
      TextLayout_c::CommandData_c c(
        a.attribute("url").value(),
        std::stoi(a.attribute("x").value()),
        std::stoi(a.attribute("y").value()),
        std::stoi(a.attribute("w").value()),
        std::stoi(a.attribute("h").value())
      );

      l.addCommand(c);
    }
  }

  l.setHeight(std::stoi(doc.attribute("height").value()));
  l.setLeft(std::stoi(doc.attribute("left").value()));
  l.setRight(std::stoi(doc.attribute("right").value()));

  // load links
  // output links
  auto links = doc.child("links");
  if (links)
  {
    for (const auto l2 : links.children())
    {
      TextLayout_c::linkInformation link;

      link.url = l2.attribute("url").value();

      for (const auto a : l2.children())
      {
        if (a.name() == std::string("area"))
        {
          TextLayout_c::rectangle_c r;
          r.x = std::stoi(a.attribute("x").value());
          r.y = std::stoi(a.attribute("y").value());
          r.w = std::stoi(a.attribute("w").value());
          r.h = std::stoi(a.attribute("h").value());

          link.areas.push_back(r);
        }
      }

      l.links.push_back(link);
    }
  }

  return l;
}

}
