
#include <stll/color.h>

// lookup tables for gamma correct output
// gamma contains the gamma value that the lookup tables are currently
// set up for
// GAMMA_SCALE is a scaling factor for limit calculation errors, the bigger
// the more exact the output will be, but the bigger the 2nd lookup has to
// be, and then 2 lookup tables for forward and inverse correction
// the tables are updated when showLayout is called with a new gamma value
template <int S>
class Gamma_c {

  private:

    uint8_t gamma = 0;
    uint16_t gammaFor[256] = {};
    uint16_t gammaInv[256*S] = {};

  public:

    Gamma_c(void) { }

    void setGamma(uint8_t g)
    {
      if (g != gamma)
      {
        gamma = g;

        for (int i = 0; i < 256; i++)
          gammaFor[i] = (256*S-1)*pow(i/255.0, g*0.1);

        for (int i = 0; i < 256*S; i++)
          gammaInv[i] = 255*pow(i/(1.0*256*S-1), 10.0/g);
      }
    }

    STLL::Color_c forward(STLL::Color_c c) const
    {
      return STLL::Color_c(
        gammaFor[c.r()]/S,
        gammaFor[c.g()]/S,
        gammaFor[c.b()]/S,
        c.a());
    }

    uint16_t forward(uint8_t v) const { return gammaFor[v]; }
    uint8_t inverse(uint16_t v) const { return gammaInv[v]; }
    uint16_t scale(void) const { return S; }
};