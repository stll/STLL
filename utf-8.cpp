#include "utf-8.h"

#define UNI_MAX_LEGAL_UTF32 (char32_t)0x0010FFFF

/*
 * Index into the table below with the first byte of a UTF-8 sequence to
 * get the number of trailing bytes that are supposed to follow it.
 * Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
 * left as-is for anyone who may want to do such conversion, which was
 * allowed in earlier algorithms.
 */
static const char trailingBytesForUTF8[256] =
{
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/*
 * Magic values subtracted from a buffer value during UTF8 conversion.
 * This table contains as many values as there might be trailing bytes
 * in a UTF-8 sequence.
 */
static const char32_t offsetsFromUTF8[6] =
{
  0x00000000UL, 0x00003080UL, 0x000E2080UL,
  0x03C82080UL, 0xFA082080UL, 0x82082080UL
};


/* based on the valid_utf8 routine from the PCRE library by Philip Hazel
 *
 *  length is in bytes, since without knowing whether the string is valid
 *  it's hard to know how many characters there are! */
bool u8_isValid(const std::string & str)
{
  size_t len = str.length();

  for (size_t pos = 0; pos < len; pos++)
  {
    unsigned char c = str[pos];

    // single byte variation
    if (c < 128)
      continue;

    // in all other cases a multi byte sequence starts, it must start with a special character
    if ((c & 0xc0) != 0xc0)
      return 0;

    // how many additional bytes are there?
    int ab = trailingBytesForUTF8[c];

    // not enough characters left for the required multibytes
    if (pos + ab >= len)
      return false;

    char32_t ch = 0;

    /*
     * The cases all fall through. See "Note A" below.
     */
    switch (ab)
    {
      case 5: ch += str[pos]; pos++; ch <<= 6;
      case 4: ch += str[pos]; pos++; ch <<= 6;
      case 3: ch += str[pos]; pos++; ch <<= 6;
      case 2: ch += str[pos]; pos++; ch <<= 6;
      case 1: ch += str[pos]; pos++; ch <<= 6;
      case 0: ch += str[pos]; pos++;
    }
    ch -= offsetsFromUTF8[ab];

    if      (ch < (char32_t)0x80)       { if (ab != 1) { return false; } }
    else if (ch < (char32_t)0x800)      { if (ab != 2) { return false; } }
    else if (ch < (char32_t)0x10000)    { if (ab != 3) { return false; } }
    else if (ch <= UNI_MAX_LEGAL_UTF32) { if (ab != 4) { return false; } }
    else {                                               return false; }
  }

  return 1;
}

std::u32string u8_convertToU32(const std::string & in)
{
  size_t pos = 0;
  size_t len = in.length();
  std::u32string out;

  while (pos < len)
  {
    char32_t ch = 0;

    int extraBytesToRead = trailingBytesForUTF8[(uint8_t)in[pos]];

    if (pos + extraBytesToRead >= len)
    {
      // exhausted
      break;
    }

    /*
     * The cases all fall through. See "Note A" below.
     */
    switch (extraBytesToRead) {
      case 5: ch += (uint8_t)in[pos]; pos++; ch <<= 6;
      case 4: ch += (uint8_t)in[pos]; pos++; ch <<= 6;
      case 3: ch += (uint8_t)in[pos]; pos++; ch <<= 6;
      case 2: ch += (uint8_t)in[pos]; pos++; ch <<= 6;
      case 1: ch += (uint8_t)in[pos]; pos++; ch <<= 6;
      case 0: ch += (uint8_t)in[pos]; pos++;
    }
    ch -= offsetsFromUTF8[extraBytesToRead];

    out.push_back(ch);
  }

  return out;
}