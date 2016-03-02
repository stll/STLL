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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

// based on https://github.com/hunspell/hyphen but heavily modified for C++
// and the usage required here

/* Hyphen - hyphenation library using converted TeX hyphenation patterns
 *
 * (C) 1998 Raph Levien
 * (C) 2001 ALTLinux, Moscow
 * (C) 2006, 2007, 2008 László Németh
 *
 * This was part of libHnj library by Raph Levien.
 *
 * Peter Novodvorsky from ALTLinux cut hyphenation part from libHnj
 * to use it in OpenOffice.org.
 *
 * Non-standard and compound word hyphenation support by László Németh.
 *
 * License is the original LibHnj license:
 *
 * LibHnj is dual licensed under LGPL and MPL. Boilerplate for both
 * licenses follows.
 */

/* LibHnj - a library for high quality hyphenation and justification
 * Copyright (C) 1998 Raph Levien
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307  USA.
*/

/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "MPL"); you may not use this file except in
 * compliance with the MPL.  You may obtain a copy of the MPL at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the MPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the MPL
 * for the specific language governing rights and limitations under the
 * MPL.
 *
 */
#ifndef STLL_HYPHEN_H
#define STLL_HYPHEN_H

#include <iostream>
#include <vector>
#include <memory>
#include <map>
#include <algorithm>
#include <string>
#include <utility>
#include <stll/utf-8.h>
#include <stdexcept>

namespace STLL { namespace internal {

template <class C>
struct constants { };

template <>
struct constants<char>
{
  std::string dot = ".";
  std::string empty = "";
  std::string zero = "0";
  std::string nextlevel = "NEXTLEVEL";
  std::string line1 = "1-1";
  std::string line2 = "1'1";
  std::string line3 = u8"1\u82111";
  std::string line4 = u8"1\u82171";
  std::string line5 = u8"NOHYPHEN ',\u8211,\u8217,-";
  std::string lefthyphenmin = "LEFTHYPHENMIN";
  std::string righthyphenmin = "RIGHTHYPHENMIN";
  std::string compoundlefthyphenmin = "COMPOUNDLEFTHYPHENMIN";
  std::string compoundrighthyphenmin = "COMPOUNDRIGHTHYPHENMIN";
  std::string nohyphen = "NOHYPHEN";
  std::string casefold = "CASE";

  bool getline(std::istream & f, std::string & line) const
  {
    return std::getline(f, line);
  }

  int stoi(const std::string & t) const
  {
    return std::stoi(t);
  }

  bool utf8 = true;
};

template<>
struct constants<char32_t>
{
  std::u32string dot = U".";
  std::u32string empty = U"";
  std::u32string zero = U"0";
  std::u32string nextlevel = U"NEXTLEVEL";
  std::u32string line1 = U"1-1";
  std::u32string line2 = U"1'1";
  std::u32string line3 = U"1\u82111";
  std::u32string line4 = U"1\u82171";
  std::u32string line5 = U"NOHYPHEN ',\u8211,\u8217,-";

  std::u32string lefthyphenmin = U"LEFTHYPHENMIN";
  std::u32string righthyphenmin = U"RIGHTHYPHENMIN";
  std::u32string compoundlefthyphenmin = U"COMPOUNDLEFTHYPHENMIN";
  std::u32string compoundrighthyphenmin = U"COMPOUNDRIGHTHYPHENMIN";
  std::u32string nohyphen = U"NOHYPHEN";
  std::u32string casefold = U"CASE";

  bool getline(std::istream & f, std::u32string & line) const
  {
    std::string temp;
    bool result = std::getline(f, temp);
    line = STLL::u8_convertToU32(temp);
    return result;
  }

  int stoi(const std::u32string & t) const
  {
    std::string t2;

    for (size_t i = 0; i < t.length(); i++)
    {
      if (t[i] < 0x80 && t[i] >= 0x20)
      {
        t2 += t[i];
      }
      else
      {
        break;
      }
    }

    return std::stoi(t2);
  }

  bool utf8 = false;
};

template <class C>
struct cf_constants { };

template <>
struct cf_constants<char>
{
  std::pair<char32_t, size_t> input(const std::string & in, size_t pos) const
  {
    return u8_convertFirstToU32(in, pos);
  }

  std::string output(char32_t c) const
  {
    return U32ToUTF8(c);
  }

  std::u32string convert(const std::string & in) const
  {
    return u8_convertToU32(in);
  }
};

template <>
struct cf_constants<char32_t>
{
  std::pair<char32_t, size_t> input(const std::u32string & in, size_t pos) const
  {
    return std::make_pair(in[pos], pos+1);
  }

  char32_t output(char32_t c) const
  {
    return c;
  }

  const std::u32string & convert(const std::u32string & in) const
  {
    return in;
  }
};

template <class C>
class casefolding
{
  std::vector<char32_t> lowcase;
  char32_t startindex;

  cf_constants<C> cc;

  public:
    casefolding(const std::basic_string<C> & load)
    {
      auto ll = cc.convert(load);

      if (ll.substr(0, 4) != U"LUT ")
      {
        throw std::runtime_error("Only LUT Case folding supported");
      }

      std::vector<std::pair<char32_t, char32_t>> pairs;

      for (size_t p = 4; p < ll.size(); p++)
      {
        if (ll[p] != U' ')
        {
          if (ll[p+1] == U' ' || (p+2 < ll.size() && ll[p+2] != U' '))
          {
            throw std::runtime_error("CASELUT must be always 2 characters separates by space");
          }
          pairs.push_back(std::make_pair(ll[p], ll[p+1]));

          p += 2;
        }
      }

      if (pairs.size() == 0)
      {
        throw std::runtime_error("CASELUT must contain at least one pair");
      }

      // find maximum and minimum first character
      char32_t min = pairs[0].first;
      char32_t max = min;

      for (size_t i = 1; i < pairs.size(); i++)
      {
        min = std::min(min, pairs[i].first);
        max = std::max(max, pairs[i].first);
      }

      startindex = min;
      lowcase.resize(max-min+1);

      for (auto & p : pairs)
        lowcase[p.first-min] = p.second;

      for (size_t i = 0; i < lowcase.size(); i++)
        if (lowcase[i] == 0)
          lowcase[i] = i+startindex;
    }

    std::basic_string<C> fold(const std::basic_string<C> & in) const
    {
      size_t pos = 0;
      std::basic_string<C> out;

      while (pos < in.size())
      {
        char32_t c;
        std::tie(c, pos) = cc.input(in, pos);

        if ((c >= startindex) && (c-startindex < lowcase.size()))
        {
          c = lowcase[c-startindex];
        }

        out += cc.output(c);
      }

      return out;
    }
};

// supported values for C: char and char32_t
template <class C>
class HyphenDict
{
  public:
    typedef std::basic_string<C> string;

    class Hyphens {
      public:
        const string * rep;
        uint8_t pos = 0;
        uint8_t cut = 0;
        uint8_t hyphens = '0';
    };

  private:

    const constants<C> con;

    struct HyphenTrans
    {
      HyphenTrans(C c, int n) : new_state(n), ch(c) {}

      size_t new_state;
      C ch;
    };

    struct HyphenState
    {
      std::vector<uint8_t> match;
      std::vector<HyphenTrans> trans;
      string repl;
      int fallback_state = -1;
      uint8_t replindex = 0;
      uint8_t replcut = 0;
    };

    /* user options */
    int lhmin = 0;    /* lefthyphenmin: min. hyph. distance from the left side */
    int rhmin = 0;    /* righthyphenmin: min. hyph. distance from the right side */
    int clhmin = 0;   /* min. hyph. distance from the left compound boundary */
    int crhmin = 0;   /* min. hyph. distance from the right compound boundary */
    std::vector<string> nohyphen; /* comma separated list of characters or character sequences with forbidden hyphenation */

    /* system variables */
    std::vector<HyphenState> states;
    std::unique_ptr<HyphenDict> nextlevel;

    std::unique_ptr<casefolding<C>> casefold;

    typedef std::map<string, int> HashTab;

    /* return val if found, otherwise -1 */
    int hash_lookup (const HashTab & hashtab, const string & key)
    {
      auto a = hashtab.find(key);

      if (a != hashtab.end())
        return a->second;

      return -1;
    }

    /* Get the state number, allocating a new state if necessary. */
    int get_state (HashTab & hashtab, const string & string)
    {
      int state_num = hash_lookup (hashtab, string);

      if (state_num >= 0)
        return state_num;

      hashtab.insert(std::make_pair(string, states.size()));

      states.push_back(HyphenState());

      return states.size()-1;
    }

    void load_line(const string & buf2, HashTab & hashtab)
    {
      if (buf2.compare(0, 13, con.lefthyphenmin) == 0)
      {
        lhmin = con.stoi(buf2.substr(13));
      }
      else if (buf2.compare(0, 14, con.righthyphenmin) == 0)
      {
        rhmin = con.stoi(buf2.substr(14));
      }
      else if (buf2.compare(0, 21, con.compoundlefthyphenmin) == 0)
      {
        clhmin = con.stoi(buf2.substr(21));
      }
      else if (buf2.compare(0, 22, con.compoundrighthyphenmin) == 0)
      {
        crhmin = con.stoi(buf2.substr(22));
      }
      else if (buf2.compare(0, 4, con.casefold) == 0)
      {
        casefold = std::make_unique<casefolding<C>>(buf2.substr(4));
      }
      else if (buf2.compare(0, 8, con.nohyphen) == 0)
      {
        size_t space = 8;

        // skip spaces
        while (space < buf2.length() && (buf2[space] == ' ' || buf2[space] == '\t'))
          space++;

        // separate at commas and put into string vector
        while (space < buf2.length())
        {
          size_t start = space;

          while (buf2[space] != ',' && buf2[space] != '\n' && space < buf2.length())
            space++;

          nohyphen.emplace_back(buf2.substr(start, space-start));
          space++;
        }
      }
      else
      {
        uint8_t replindex = 0;
        uint8_t replcut = 0;
        string repl;

        // when there is a slash, there should also be 2 commas, separate along them and put result
        // into replindex, replcut and repl
        auto slashpos = buf2.find_first_of('/');
        if (slashpos != string::npos)
        {
          auto commapos1 = buf2.find_first_of(',', slashpos);

          if (commapos1 != string::npos)
          {
            auto commapos2 = buf2.find_first_of(',', commapos1+1);

            if (commapos2 != string::npos)
            {
              replindex = con.stoi(buf2.substr(commapos1+1, commapos2-commapos1-1)) - 1;
              replcut = con.stoi(buf2.substr(commapos2+1));
              repl = buf2.substr(slashpos+1, commapos1-slashpos-1);
            }
          }
          else
          {
            replcut = slashpos;
            repl = buf2.substr(slashpos+1);
          }
        }

        string word;
        string pattern = con.zero;

        for (size_t i = 0; i < buf2.length() && buf2[i] != '/'; i++)
        {
          if (buf2[i] >= '0' && buf2[i] <= '9')
          {
            pattern.back() = buf2[i];
          }
          else
          {
            word += buf2[i];
            pattern += '0';
          }
        }

        size_t i = 0;
        if (repl.length() == 0)
        {
          /* Optimize away leading zeroes */
          while (pattern[i] == '0') i++;
        }
        else
        {
          if (word[0] == '.') i++;
          if (con.utf8)
          {
            // because the discretionary hyphen positions and lengths are given
            // in character lengths, we need to convert them to number of utf-8 bytes
            int pu = -1;        /* unicode character position */
            int ps = -1;        /* unicode start position (original replindex) */
            size_t pc = (word[0] == '.') ? 1: 0; /* 8-bit character position */
            for (; pc < word.length() + 1; pc++)
            {
              /* beginning of an UTF-8 character (not '10' start bits) */
              if ((((uint8_t) word[pc]) >> 6) != 2) pu++;
              if ((ps < 0) && (replindex == pu))
              {
                ps = replindex;
                replindex = (int8_t) pc;
              }
              if ((ps >= 0) && ((pu - ps) == replcut))
              {
                replcut = (int8_t) (pc - replindex);
                break;
              }
            }
            if (word[0] == '.') replindex--;
          }
        }

        int found = hash_lookup (hashtab, word);
        int state_num = get_state(hashtab, word);

        for (size_t x = 0; i+x < pattern.length(); x++)
        {
          states[state_num].match.push_back(pattern[i+x]);
        }

        states[state_num].repl = std::move(repl);
        states[state_num].replindex = replindex;
        if (replcut == 0)
        {
          states[state_num].replcut = word.length();
        }
        else
        {
          states[state_num].replcut = replcut;
        }

        /* now, put in the prefix transitions */
        while (found < 0 && word.length() > 0)
        {
          int last_state = state_num;
          C ch = word.back();
          word.resize(word.length()-1);
          found = hash_lookup (hashtab, word);
          state_num = get_state(hashtab, word);
          states[state_num].trans.push_back(HyphenTrans(ch, last_state));
        }
      }
    }

    void hyphenate_rec(const string& word, std::vector<Hyphens> & result, int clhmin, int crhmin, bool lend, bool rend) const
    {
      string prep_word(con.dot + word + con.dot);
      for (C & c : prep_word)
        if (c >= '0' && c <= '9')
          c = '.';

      result.clear();
      result.resize(prep_word.size());

      for (auto & r : result) r.rep = &con.empty;

      /* now, run the finite state machine */
      int state = 0;
      for (size_t i = 0; i < prep_word.length(); i++)
      {
        C ch = prep_word[i];
        while (true)
        {
          auto trans = std::find_if(states[state].trans.begin(), states[state].trans.end(),
              [ch](const HyphenTrans & t) -> bool { return t.ch == ch; });

          if (trans == states[state].trans.end())
          {
            state = states[state].fallback_state;

            if (state == -1)
            {
              // ok, continue with next letter
              state = 0;
              break;
            }
          }
          else
          {
            state = trans->new_state;

            /* Additional optimization is possible here - especially,
               elimination of trailing zeroes from the match. Leading zeroes
               have already been optimized. */

            auto & match = states[state].match;
            auto & repl = states[state].repl;
            unsigned int replindex = states[state].replindex;
            unsigned int replcut = states[state].replcut;

            int offset = i - match.size();
            size_t k = std::max(-offset, 0);
            size_t kend = std::min(match.size(), prep_word.length()-3-offset);

            while (k < kend)
            {
              if (result[offset + k].hyphens < match[k])
              {
                result[offset + k].hyphens = match[k];

                if ((match[k] % 2) != 0)
                {
                  result[offset + 1 + replindex].cut = replcut;
                  result[offset + 1 + k].rep = &repl;
                  if (repl.length() && (k >= replindex) && (k <= replindex + replcut))
                  {
                    result[offset + 1 + replindex].pos = offset + 1 + k;
                  }
                }
              }
              k++;
            }
            break;
          }
        }
      }

      /* now create a new char string showing hyphenation positions */
      /* count the hyphens and allocate space for the new hyphenated string */

      for (size_t i = 0; i < word.length(); i++)
      {
        if (result[i].pos > 0)
        {
          result[result[i].pos - 1].rep = result[result[i].pos].rep;
          result[result[i].pos - 1].pos = result[i].pos - i;
          result[result[i].pos - 1].cut = result[i].cut;
          i += result[i].cut - 1;
        }
      }

      // recursive hyphenation of the first (compound) level segments
      if (nextlevel)
      {
        size_t begin = 0;
        int beginofs = 0;
        string prefix;
        std::vector<Hyphens> result2;

        for (size_t i = 0; i < word.length(); i++)
        {
          if ((result[i].hyphens % 2 != 0) || (begin > 0 && i + 1 == word.length()))
          {
            if (i > begin)
            {
              /* non-standard hyphenation at compound boundary (Schiffahrt) */
              auto prep_word2 = prefix + prep_word.substr(begin+beginofs+1, i-begin-beginofs+1-result[i].pos) +
                                result[i].rep->substr(0, result[i].rep->find_first_of('='));

              hyphenate_rec(prep_word2, result2, clhmin, crhmin, begin == 0 && lend, result[i].hyphens % 2 == 0 && rend);

              std::copy(result2.begin(), result2.begin()+(i-begin), result.begin()+begin);
            }
            begin = i + 1;
            beginofs = result[i].cut - result[i].pos;
            prefix = result[i].rep->substr(result[i].rep->find('=')+1);
          }
        }

        // non-compound
        if (begin == 0)
        {
          nextlevel->hyphenate_rec(word, result, clhmin, crhmin, lend, rend);
          if (!lend) hnj_hyphen_lhmin(con.utf8, word, result, clhmin);
          if (!rend) hnj_hyphen_rhmin(con.utf8, word, result, crhmin);
        }
      }
    }

    HyphenDict(void) {}

    /* Unicode ligature length */
    static int hnj_ligature(C c)
    {
      switch (c) {
        case '\x80':              /* ff */
        case '\x81':              /* fi */
        case '\x82': return 0;    /* fl */
        case '\x83':              /* ffi */
        case '\x84': return 1;    /* ffl */
        case '\x85':              /* long st */
        case '\x86': return 0;    /* st */
      }
      return 0;
    }

    /* character length of the first n byte of the input word */
    static int hnj_hyphen_strnlen(const string & word, size_t n, bool utf8)
    {
      int i = 0;
      size_t j = 0;

      n = std::min(n, word.length());

      while (j < n)
      {
        i++;
        // Unicode ligature support
        if (utf8 && ((uint8_t) word[j] == 0xEF) && ((uint8_t) word[j + 1] == 0xAC))
        {
          i += hnj_ligature(word[j + 2]);
        }
        for (j++; utf8 && (word[j] & 0xc0) == 0x80; j++);
      }

      return i;
    }

    int hnj_hyphen_lhmin(bool utf8, const string & word, std::vector<HyphenDict::Hyphens> & hyphens, int lhmin) const
    {
      int i = 1;

      // Unicode ligature support
      if (utf8 && ((uint8_t) word[0] == 0xEF) && ((uint8_t) word[1] == 0xAC))
      {
        i += hnj_ligature(word[2]);
      }

      // ignore numbers
      for (int j = 0; word[j] <= '9' && word[j] >= '0'; j++) i--;

      for (size_t j = 0; i < lhmin && j < word.length(); i++)
        do
        {
          // check length of the non-standard part
          if (hyphens[j].rep->length() > 0)
          {
            size_t rh = hyphens[j].rep->find_first_of('=');
            if (rh != std::string::npos && (hnj_hyphen_strnlen(word, j - hyphens[j].pos + 1, utf8) + hnj_hyphen_strnlen(*hyphens[j].rep, rh, utf8)) < lhmin)
            {
              hyphens[j].rep = &con.empty;
              hyphens[j].hyphens = '0';
            }
          }
          else
          {
            hyphens[j].hyphens = '0';
          }
          j++;

          // Unicode ligature support
          if (utf8 && ((uint8_t) word[j] == 0xEF) && ((uint8_t) word[j + 1] == 0xAC))
          {
            i += hnj_ligature(word[j + 2]);
          }
        } while (utf8 && (word[j] & 0xc0) == 0x80);

      return 0;
    }

    int hnj_hyphen_rhmin(bool utf8, const string & word, std::vector<HyphenDict::Hyphens> & hyphens, int rhmin) const
    {
      int i = 0;

      // ignore numbers
      for (int j = word.length() - 1; j > 0 && word[j] <= '9' && word[j] >= '0'; j--) i--;

      for (int j = word.length() - 1; i < rhmin && j > 0; j--)
      {
        // check length of the non-standard part
        if (hyphens[j].rep->length() > 0)
        {
          size_t rh = hyphens[j].rep->find_first_of('=');
          if (rh != std::string::npos && (hnj_hyphen_strnlen(word.substr(j - hyphens[j].pos + hyphens[j].cut + 1), 100, utf8) +
                hnj_hyphen_strnlen(hyphens[j].rep->substr(rh + 1), hyphens[j].rep->length()-(rh + 1), utf8)) < rhmin)
          {
            hyphens[j].rep = &con.empty;
            hyphens[j].hyphens = '0';
          }
        }
        else
        {
          hyphens[j].hyphens = '0';
        }
        if (!utf8 || (word[j] & 0xc0) == 0xc0 || (word[j] & 0x80) != 0x80) i++;
      }

      return 0;
    }

  public:

    /** \brief create a new hyphenation dictionary by reading the given stream
     *
     * \param f stream to read. The stream must be an openoffice dictionary, but only
     * UTF-8 encoded dictionaries are supported
     */
    HyphenDict(std::istream & f)
    {
      // this is a bit of magic that took me a while to understand:
      // I create a derived class from HyphenDict that does have a default
      // constructor. I can only do this because here I am inside of
      // Hyphen Dict and I do have access to the default constructor
      // of HyphenDict. Someone else will not be able to achieve this
      struct make_unique_enabler : public HyphenDict<C> {};

      std::unique_ptr<HyphenDict<C>> dict[2];

      bool nextlevelvalid = false;

      // loading one or two dictionaries (separated by NEXTLEVEL keyword)
      for (int k = 0; k < 2; k++)
      {
        HashTab hashtab;
        hashtab.insert(std::make_pair(con.empty, 0));
        dict[k] = std::make_unique<make_unique_enabler>();
        dict[k]->states.push_back(HyphenState());

        /* read in character set info */
        if (k == 0)
        {
          std::string line;
          std::getline(f, line);
          if (line.compare("UTF-8") != 0)
          {
            throw std::runtime_error("Only utf-8 formatted hyphen dictionaries are supported by STLL hyphen support");
          }
        }

        if (k == 0 || nextlevelvalid)
        {
          string line;

          while (con.getline(f, line))
          {
            if (line.compare(0, 9, con.nextlevel) == 0)
            {
              nextlevelvalid = true;
              break;
            }
            else if (line[0] != '%')
            {
              dict[k]->load_line(line, hashtab);
            }
          }
        }
        else if (k == 1)
        {
          /* default first level: hyphen and ASCII apostrophe */

          dict[k]->load_line(con.line1, hashtab); /* remove hyphen */
          dict[k]->load_line(con.line2, hashtab); /* ASCII apostrophe */
          dict[k]->load_line(con.line3, hashtab); /* endash */
          dict[k]->load_line(con.line4, hashtab); /* apostrophe */
          dict[k]->load_line(con.line5, hashtab);
        }

        /* Could do unioning of matches here (instead of the preprocessor script).
           If we did, the pseudocode would look something like this:

           foreach state in the hash table
           foreach i = [1..length(state) - 1]
           state to check is substr (state, i)
           look it up
           if found, and if there is a match, union the match in.

           It's also possible to avoid the quadratic blowup by doing the
           search in order of increasing state string sizes - then you
           can break the loop after finding the first match.

           This step should be optional in any case - if there is a
           preprocessed rule table, it's always faster to use that.

         */

        /* put in the fallback states */

        for (auto & e : hashtab)
        {
          if (e.first.length() > 0)
          {
            int state_num = 0;
            int j = 1;

            while (true)
            {
              state_num = hash_lookup(hashtab, e.first.substr(j));

              if (state_num >= 0)
              {
                dict[k]->states[e.second].fallback_state = state_num;
                break;
              }

              j++;
            }
          }
        }
      }

      if (nextlevelvalid)
      {
        dict[0]->nextlevel = std::move(dict[1]);
        *this = std::move(*dict[0]);
      }
      else
      {
        dict[1]->lhmin = dict[0]->lhmin;
        dict[1]->rhmin = dict[0]->rhmin;
        dict[1]->clhmin = (dict[0]->clhmin) ? dict[0]->clhmin : ((dict[0]->lhmin) ? dict[0]->lhmin : 3);
        dict[1]->crhmin = (dict[0]->crhmin) ? dict[0]->crhmin : ((dict[0]->rhmin) ? dict[0]->rhmin : 3);
        dict[1]->casefold = std::move(dict[0]->casefold);
        dict[1]->nextlevel = std::move(dict[0]);
        *this = std::move(*dict[1]);
      }
    }

    HyphenDict(HyphenDict && d)
    {
      *this = std::move(d);
    }

    HyphenDict & operator=(HyphenDict && d)
    {
      lhmin = d.lhmin;
      rhmin = d.rhmin;
      clhmin = d.clhmin;
      crhmin = d.crhmin;

      nohyphen = std::move(d.nohyphen);
      states = std::move(d.states);
      nextlevel = std::move(d.nextlevel);
      casefold = std::move(d.casefold);

      return *this;
    }

    /**
     * \brief hyphenate a word
     *
     * \param word, the word to hyphenate
     * \param result the result of the hyphenation
     * \param lhmin_ minimum number of glyphes from the start of the word to the first hyphen
     * \param rhmin_ minimum number of glyphes from the last hyphen to the end of the word
     * \param clhmin_ same as lhmin, but used for compound words, like in German,
     * \param crhmin_ same as clhmin but for the word end
     *
     * The lmin and rmin parameters overwrite (in the form of "take the maximum") the values for
     * these that are provided by the dictionary
     *
     * The output is a bit hard to interpret, so here are the details. The vector can be reused
     * for a nice speed-up, so keep reinserting it. It consists of the following fields
     * - hyphens: an number, if odd you may break here
     * - rep: a replacement text to use when you break here. if empty, just insert the hyphen and be done, else
     *        you insert this text
     * - pos: but before you can do this go pos letters back and remove cut letters from the word
     * - cut
     *
     * so assume you have word w and want to use the hyphen at position p. The resulting word
     * would be w.substring(0, i-pos) + rep + w.substring(0, i-pos+cut)
     */
    void hyphenate(string word, std::vector<Hyphens> & result,
        int lhmin_ = 0, int rhmin_ = 0, int clhmin_ = 0, int crhmin_ = 0) const
    {
      lhmin_ = std::max(lhmin_, lhmin);
      rhmin_ = std::max(rhmin_, rhmin);
      clhmin_ = std::max(clhmin_, clhmin);
      crhmin_ = std::max(crhmin_, crhmin);

      if (lhmin_ <= 0) lhmin_ = 2;
      if (rhmin_ <= 0) rhmin_ = 2;

      if (casefold)
      {
        hyphenate_rec(casefold->fold(word), result, clhmin_, crhmin_, true, true);
      }
      else
      {
        hyphenate_rec(word, result, clhmin_, crhmin_, true, true);
      }

      hnj_hyphen_lhmin(con.utf8, word, result, lhmin_);
      hnj_hyphen_rhmin(con.utf8, word, result, rhmin_);

      /* nohyphen */
      for (const auto & nh : nohyphen)
      {
        size_t nhy = word.find(nh);
        while (nhy != string::npos)
        {
          result[nhy + nh.length() - 1].hyphens = '0';
          if (nhy > 0) result[nhy - 1].hyphens = '0';
          nhy = word.find(nh, nhy + 1);
        }
      }
    }
};


} }



#endif
