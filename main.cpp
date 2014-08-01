#include "layouterXHTML.h"
#include "layouterSDL.h"

#define TXT_WIDTH 700
#define WIN_WIDTH 800

int main ()
{
  textStyleSheet_c styleSheet;

  // setze default sprache des textes
  styleSheet.language="en-Engl";

  // alle Fonts, die so genutzt werden: familie heißt sans, und dann der bold Font dazu
  styleSheet.font("sans", "/usr/share/fonts/freefont/FreeSans.ttf");
  styleSheet.font("sans", "/usr/share/fonts/freefont/FreeSansBold.ttf", "normal", "normal", "bold");

  // CSS regeln, immer Selector, attribut, wert
  styleSheet.addRule("body", "color", "#ffffff");
  styleSheet.addRule("body", "font-family", "sans");
  styleSheet.addRule("body", "font-style", "normal");
  styleSheet.addRule("body", "font-size", "30px");
  styleSheet.addRule("body", "font-variant", "normal");
  styleSheet.addRule("body", "font-weight", "normal");
  styleSheet.addRule("body", "text-align", "justify");
  styleSheet.addRule(".BigFont", "font-size", "35px");
  styleSheet.addRule(".BoldFont", "font-weight", "bold");
  styleSheet.addRule("i", "color", "#ff0000");
  styleSheet.addRule("h1", "font-weight", "bold");
  styleSheet.addRule("h1", "font-size", "60px");
  styleSheet.addRule("h1", "text-align", "center");

  // der zu formatierende text
  std::string text = u8"<html><body>"
    "<h1>Überschrift</h1>"
    "<p>Test <i>Text</i> more and some "
    "<div class='BoldFont'>more text so</div> that the pa\u00ADra\u00ADgraph is at least "
    "<div>long</div> enough to span some lines on the screen let us "
    "also <i>i</i>nclude sm hebrew נייה, העגורן הוא and back to english</p>"
    "<p>2nd Paragraph<div class='BigFont'> with <div class='BoldFont'>a\ndivision</div>"
    "</div>.</p>"
    "<p>a b c andnowone<i>very</i>longwordthatdoesntfitononelineandmight d e f °C</p>"
    "<ul><li>First long text in an ul list of html sdfsd fsd fs dfsd f gobble di gock and"
    "even more</li><li>Second with just a bit</li></ul>"
    "<p>Margaret­Are­You­Grieving­Over­Goldengrove­Unleaving­Leaves­Like­The­Things­Of­Man­You­With­Your­Fresh­Thoughts­Care­For­Can­You­Ah­As­The­Heart­Grows­Older­It­Will­Come­To­Such­Sights­Colder­By­And­By­Nor­Spare­A­Sigh­Though­Worlds­Of­Wanwood­Leafmeal­Lie­And­Yet­You­Will­Weep­And­Know­Why­Now­No­Matter­Child­The­Name­Sorrows­Springs­Are­The­Same­Nor­Mouth­Had­No­Nor­Mind­Expressed­What­Heart­Heard­Of­Ghost­Guessed­It­Is­The­Blight­Man­Was­Born­For­It­Is­Margaret­You­Mourn­For</p>"
    "</body></html>";

  // Vektor mit auszugebenden Text layouts, besteht immer aus einem layoute
  // und dessen Position auf dem Bildschirm
  std::vector<layoutInfo_c> l;

  // grauer Hintergrund, damit ich sehen kann, ob der Text die korrekte Breite hat
  // erzeuge ein künstliches Layout nur mit einem Rechteck drin... so etwas wird später
  // nicht mehr benötigt, ist nur für den Test
  textLayout_c la;
  textLayout_c::commandData c;
  c.command = textLayout_c::commandData::CMD_RECT;
  c.x = c.y = 0;
  c.w = TXT_WIDTH;
  c.h = 600;
  c.r = c.g = c.b = 50;
  la.addCommand(c);
  l.emplace_back(layoutInfo_c(la, (WIN_WIDTH-TXT_WIDTH)/2, 20));

  // das eigentliche Layout
  // layoutXHTML macht die Arbeit, übergeben wird der Text, das Stylesheet und eine Klasse,
  // die die Form des Textes beinhaltet (für nicht rechteckiges Layout
  l.emplace_back(layoutInfo_c(layoutXHTML(text, styleSheet, rectangleShape_c(TXT_WIDTH)),
                              (WIN_WIDTH-TXT_WIDTH)/2, 20));

  // Ausgabe mittels SDL
  showLayoutsSelf(WIN_WIDTH, 600, l);
}
