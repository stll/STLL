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
#include <stll/utf-8.h>

#include <assert.h>

namespace STLL {

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


/* based on the valid_utf8 routine from the PCRE library by Philip Hazel so if you wish
 * you may also apply the licence of that library which is as follows
 */

/* PCRE is a library of functions to support regular expressions whose syntax
 a n*d semantics are as close as possible to those of the Perl 5 language.

 Written by Philip Hazel
 Copyright (c) 1997-2012 University of Cambridge

 -----------------------------------------------------------------------------
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.

 * Neither the name of the University of Cambridge nor the names of its
 contributors may be used to endorse or promote products derived from
 this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 -----------------------------------------------------------------------------
 */
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

    int extraBytesToRead = trailingBytesForUTF8[static_cast<uint8_t>(in[pos])];

    if (pos + extraBytesToRead >= len)
    {
      // exhausted
      break;
    }

    /*
     * The cases all fall through. See "Note A" below.
     */
    switch (extraBytesToRead) {
      case 5: ch += static_cast<uint8_t>(in[pos]); pos++; ch <<= 6;
      case 4: ch += static_cast<uint8_t>(in[pos]); pos++; ch <<= 6;
      case 3: ch += static_cast<uint8_t>(in[pos]); pos++; ch <<= 6;
      case 2: ch += static_cast<uint8_t>(in[pos]); pos++; ch <<= 6;
      case 1: ch += static_cast<uint8_t>(in[pos]); pos++; ch <<= 6;
      case 0: ch += static_cast<uint8_t>(in[pos]); pos++;
    }
    ch -= offsetsFromUTF8[extraBytesToRead];

    out.push_back(ch);
  }

  return out;
}

std::string U32ToUTF8(char32_t ch)
{
  std::string result;

  if (ch < 0x80)
  {
    result += static_cast<char>(ch);
  }
  // U+0080..U+07FF
  else if (ch < 0x800)
  {
    result += static_cast<char>(0xC0 | (ch >> 6));
    result += static_cast<char>(0x80 | (ch & 0x3F));
  }
  // U+0800..U+FFFF
  else if (ch < 0x10000)
  {
    result += static_cast<char>(0xE0 | (ch >> 12));
    result += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
    result += static_cast<char>(0x80 | (ch & 0x3F));
  }
  else
  {
    // not supported, yet
    assert(0);
  }

  return result;
}



}
