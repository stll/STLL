#include "layouterXHTML.h"
#include "layouterSDL.h"
#include <boost/concept_check.hpp>

#include <fstream>

#define TXT_WIDTH 700
#define WIN_WIDTH 800

using namespace STLL;

class niftyShape_c : public shape_c
{
public:
  niftyShape_c(void) {}

  virtual int32_t getLeft(int32_t top, int32_t /*bottom*/) const { return top/4; }
  virtual int32_t getRight(int32_t top, int32_t /*bottom*/) const { return TXT_WIDTH-top/8; }
};

std::pair<std::shared_ptr<uint8_t>, size_t> loadFile(const std::string & fn)
{
  std::ifstream f(fn.c_str());

  f.seekg(0, std::ios_base::end);

  auto size = f.tellg();

  uint8_t * dat = new uint8_t[size];

  f.seekg(0, std::ios_base::beg);

  f.read((char*)dat, size);

  return std::make_pair(std::shared_ptr<uint8_t>(dat, std::default_delete<uint8_t[]>()), size);
}

void showLayoutsSelf(int w, int h, const std::vector<layoutInfo_c> & data)
{
  /* Initialize our SDL window */
  if(SDL_Init(SDL_INIT_VIDEO) < 0)   {
    fprintf(stderr, "Failed to initialize SDL");
    return;
  }

  SDL_Surface *screen;
  screen = SDL_SetVideoMode(w, h, 32, SDL_SWSURFACE | SDL_DOUBLEBUF);

  /* Enable key repeat, just makes it so we don't have to worry about fancy
   * scanboard keyboard input and such */
  SDL_EnableKeyRepeat(300, 130);
  SDL_EnableUNICODE(1);

  /* Clear our surface */
  SDL_FillRect(screen, NULL, 0 );

  for (auto & a : data)
    showLayoutSDL(a.layout, a.sx, a.sy, screen);

  SDL_Flip(screen);

  /* Our main event/draw loop */
  int done = 0;

  while (!done)
  {

    /* Handle SDL events */
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_KEYDOWN:
        case SDL_QUIT:
          done = 1;
          break;
      }
    }

    SDL_Delay(150);
  }

  SDL_Quit();
}


int main ()
{
  textStyleSheet_c styleSheet;

  // alle Fonts, die so genutzt werden: familie heißt sans, und dann der bold Font dazu
  styleSheet.font("sans", fontResource_c("/usr/share/fonts/freefont/FreeSans.ttf"));
  styleSheet.font("sans", fontResource_c("/usr/share/fonts/freefont/FreeSansBold.ttf"), "normal", "normal", "bold");
  styleSheet.font("sans-ar", fontResource_c(loadFile("fonts/amiri-0.104/amiri-regular.ttf"), "arabic"));

  // CSS regeln, immer Selector, attribut, wert
  styleSheet.addRule("body", "color", "#ffffff");
  styleSheet.addRule("body", "font-size", "18px");
  styleSheet.addRule("body", "text-align", "justify");
  styleSheet.addRule("p", "text-indent", "10px");
  styleSheet.addRule("body", "padding", "10px");
  styleSheet.addRule("body", "background-color", "#303030");
//  styleSheet.addRule("p", "padding", "10px");
//  styleSheet.addRule("ul", "padding", "10px");
//  styleSheet.addRule("li", "padding", "10px");
  styleSheet.addRule(".BigFont", "font-size", "25px");
  styleSheet.addRule(".BoldFont", "font-weight", "bold");
  styleSheet.addRule(".RTL", "text-align-last", "right");
  styleSheet.addRule("i", "color", "#ff0000");
  styleSheet.addRule("h1", "font-weight", "bold");
  styleSheet.addRule("h1", "font-size", "60px");
  styleSheet.addRule("h1", "text-align", "center");
  styleSheet.addRule("h1", "text-decoration", "underline");
  styleSheet.addRule(".und", "text-decoration", "underline");
  styleSheet.addRule("p[lang|=he]", "direction", "rtl");
  styleSheet.addRule("ul[lang|=he]", "direction", "rtl");
  styleSheet.addRule("p[lang|=ar]", "direction", "rtl");
  styleSheet.addRule("p[lang|=ar]", "font-family", "sans-ar");
  styleSheet.addRule(".framed", "border-width", "3px");
  styleSheet.addRule(".framed", "padding", "10px");
  styleSheet.addRule(".framed", "border-color", "#ff0000");
  styleSheet.addRule(".framed", "background-color", "#ff8080");
  styleSheet.addRule(".framed", "margin", "10px");

  // der zu formatierende text
  std::string text = u8"<html><body>"
    "<h1 lang='de'>Überschrift mit</h1>"
    "<p class='und'>Test <i class='BigFont' class='und'>Text</i> more and some "
    "<div class='BoldFont' class='und'>more text so</div> that the pa\u00ADra\u00ADgraph is at least "
    "<div>long</div> enough to span some lines on the screen let us "
    "also <i>i</i>nclude some more hebrew נייה, העגורן הוא and back to english </p>"
    "<p lang='he' class='framed'>אברהם בן שמואל אבולעפיה (1240 - 1291 בערך), רב ומקובל שראה עצמו כנביא וכמשיח, "
    "נחשב לרוב כנציג הבולט של זרם הקבלה האקסטטית. היה בין הראשונים שהחלו במסע לגילוי עשרת "
    "השבטים האבודים. עקב חרם שהוטל עליו על ידי הרשבא בעקבות הכרזתו על עצמו כמשיח, נשתכח אבולעפיה "
    " מלב, עד השינוי שהחל במאה השש-עשרה בו החלו הרמק ורבי חיים ויטאל להתייחס לכתביו. להכרה מלאה "
    "זכה, במאה השמונה-עשרה, עם התייחסותו החיובית של החידא לספרו הקבלי של אבולעפיה, חיי העולם הבא. "
    "אבולעפיה חיבר עשרות ספרי קבלה, שזכו להתייחסות נרחבת גם בקרב קהיליית החוקרים המדעית. חלקם יצאו "
    "לאור לאחרונה במהדורות מחודשות.</p>"
    "<p lang='ar-arab'>كأس الأمم الأفريقية لكرة القدم 2008، ,عرفت أيضاً باسم إم.تي.إن كأس الأمم الأفريقية "
    "لكرة <div class='und'>القدم بسبب الشركة</div> القدم بسبب الشركة الراعية للبطولة إم.تي.إن، كانت النسخة السادسة والعشرون من كأس الأمم الأفريقية، "
    "وهي البطولة الرئيسية لمنتخبات الاتحاد الأفريقي لكرة القدم (كاف) والتي تقام كل سنتين منذ سنة "
    "1957. نظمت المسابقة في أربعة مدن في جميع أنحاء غانا بين 20 يناير و 10 فبراير 2008. وبحضور 16 منتخبا "
    "أفريقيا مقسمين على 4 مجموعات ويتأهل أول فريقين لكل مجموعة إلى دور الثمانية حيث الأدوار الإقصائية، "
    "وكانت مباراة الأفتتاح بين غانا وغينيا والتي انتهت بفوز غانا 2-1، وانتهت البطولة بفوز مصر بالبطولة "
    "القارية، بعد أن هزمت الكاميرون 1–0 في النهائي الذي أقيم علي ملعب أوهين دجان بمدينة أكرا، بينما فاز "
    "منخب غانا بالمركز الثالث عقب تغلبه على فريق ساحل العاج بأربعة أهداف مقابل هدفين. (تابع القراءة)</p>"
    "<p>2nd Paragraph<div class='BigFont'> with <div class='BoldFont'>a\ndivision</div>"
    "</div>.</p>"
    "<p class='und'>a b c andnowone<i>very</i>longwordthatdoesntfitononelineandmight d e f °C</p>"
    "<ul class='framed'><li>First <div class='und'>long</div> text in an ul list of html sdfsd fsd fs dfsd f gobble di gock and "
    "even more</li><li>Second with just a bit</li></ul>"
    "<ul lang='he' class='framed'><li>First long text in an ul list of html sdfsd fsd fs dfsd f gobble di gock and "
    "even more</li><li>Second with just a bit</li></ul>"
    "<p>Margaret­Are­You­Grieving­Over­Goldengrove­Unleaving­Leaves­Like­The­Things­Of­Man­You­With­Your­F"
    "resh­Thoughts­Care­For­Can­You­Ah­As­The­Heart­Grows­Older­It­Will­Come­To­Such­Sights­Colder­By­And­By­Nor"
    "­Spare­A­Sigh­Though­Worlds­Of­Wanwood­Leafmeal­Lie­And­Yet­You­Will­Weep­And­Know­Why­Now­No­Matter­Child"
    "­The­Name­Sorrows­Springs­Are­The­Same­Nor­Mouth­Had­No­Nor­Mind­Expressed­What­Heart­Heard­Of­Ghost­Gues"
    "sed­It­Is­The­Blight­Man­Was­Born­For­It­Is­Margaret­You­Mourn­For</p>"
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
  l.emplace_back(layoutInfo_c(la, (WIN_WIDTH-TXT_WIDTH)/2, 10));

  try {
    // das eigentliche Layout
    // layoutXHTML macht die Arbeit, übergeben wird der Text, das Stylesheet und eine Klasse,
    // die die Form des Textes beinhaltet (für nicht rechteckiges Layout
    l.emplace_back(layoutInfo_c(layoutXHTML(text, styleSheet, rectangleShape_c(TXT_WIDTH)),
                                (WIN_WIDTH-TXT_WIDTH)/2, 10));

    l[0].layout.data[0].h = l[1].layout.getHeight();

    // Ausgabe mittels SDL
    showLayoutsSelf(WIN_WIDTH, 1000, l);
  }
  catch (XhtmlException_c & e)
  {
    printf("Xhtml Exception: %s\n", e.what());
  }
}
