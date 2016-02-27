#define USE_PUGI_XML
#include <stll/layouterXHTML.h>
#include <stll/output_SDL.h>
#include <stll/hyphendictionaries.h>

#include <stll/hyphenationdictionaries/hyph_en_US.h>
#include <stll/hyphenationdictionaries/hyph_de_DE_gz.h>

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

using namespace STLL;

int main()
{
  // all the stuff you already know to setup the style sheet
  TextStyleSheet_c styleSheet;
  styleSheet.addFont("sans", FontResource_c("tests/FreeSans.ttf"));
  styleSheet.addFont("sans", FontResource_c("tests/FreeSansBold.ttf"), "normal", "normal", "bold");
  styleSheet.addRule("body", "color", "#ffffff");
  styleSheet.addRule("body", "font-size", "20px");
  styleSheet.addRule("body", "text-align", "justify");
  styleSheet.addRule("body", "padding", "10px");
  styleSheet.addRule("h1", "font-weight", "bold");
  styleSheet.addRule("h1", "font-size", "60px");
  styleSheet.addRule("h1", "text-align", "center");
  styleSheet.addRule("h1", "background-color", "#FF8080");

  // The following lines demonstrate 2 possible ways to register hyphenation
  // dictionaries.
  // both use the provided hyphenation dictionaries that come with STLL
  // to use them you need to include the headers containing the dictionaries.
  // the included dictionaries are provided in 2 different forms: as a string
  // that contains the whole dictionary
  // and as n byte array containing the dictionary compressed using zlib.
  // if you already have to ling against zlib anyway that might be useful
  // you only pay for the dictionaries you are actually using as the
  // dictionaries are only within the header.
  // It is your responsibility to only include the headers once or your program
  // will be unnecessarily big.So a good practice for your program would be to
  // have a module that sets up all the required dictionaries.

  // us this line is the probably best way to load the uncompressed dictionary
  // you can provide multiple languages to register the dictionary for, but make
  // sure to not load the same dictionary twice as that would waste your memory
  // also you can use country code.
  STLL::addHyphenDictionary({"en", "en-us"}, std::istringstream((const char*)hyph_en_US));

  // for zlib there is no one right way. The following lines demonstrate using boosts
  // iostream library, but there are surely others out there
  {
    boost::iostreams::filtering_istream in;
    in.push(boost::iostreams::gzip_decompressor());
    boost::iostreams::array_source src((const char*)hyph_de_DE_gz, hyph_de_DE_gz_len);
    in.push(src);

    STLL::addHyphenDictionary({"de"}, in);
  }

  // The XHTML code we want to format, you will need to supply language
  // tags or otherwise nothing will be hyphenated. Texts without language
  // tag are never hyphenated
  // when the code is looking for a dictionary to use, it will first look for
  // the most specific, but then it will start stripping away elements from the specification
  // until is finds a dictionary, so it will first look for de-de, then for a dictionary
  // for the language de
  std::string text = u8"<html><body>"
    "<p lang='en'>Text in English language</p>"
    "<p lang='de-de'>Text in deutscher Sprache</p></body></html>";

  // use a 100 pixel wide rectangle so that we actually see hyphenation
  auto layout = layoutXHTMLPugi(text, styleSheet, RectangleShape_c(100*64));

  // output using SDL as already known
  SDL_Surface *screen = SDL_SetVideoMode(800, 600, 32, SDL_SWSURFACE | SDL_DOUBLEBUF);
  showSDL<> show;
  show.showLayout(layout, 20*64, 0*64, screen, SUBP_RGB);
  SDL_Flip(screen);
  sleep(10);

  return 0;
}
