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
#include <stll/hyphendictionaries.h>
#include "hyphendictionaries_internal.h"

#include "hyphen/hyphen.h"

#include <map>
#include <memory>

namespace STLL {

static std::map<std::string, std::shared_ptr<internal::HyphenDict<char32_t>>> dictionaries;

void addHyphenDictionary(const std::vector<std::string> & langs, std::istream & str)
{
  auto dict = std::make_shared<internal::HyphenDict<char32_t>>(str);
  for (auto & l : langs) dictionaries[l] = dict;
}

void addHyphenDictionary(const std::vector<std::string> & langs, std::istream && str)
{
  auto dict = std::make_shared<internal::HyphenDict<char32_t>>(str);
  for (auto & l : langs) dictionaries[l] = dict;
}

namespace internal {

const HyphenDict<char32_t> * getHyphenDict(const std::string & lang)
{
  auto d = dictionaries.find(lang);

  if (d != dictionaries.end())
  {
    return d->second.get();
  }

  auto p = lang.find_last_of('_');

  if (p != lang.npos)
  {
    std::string lang2 = lang.substr(0, p);

    auto d2 = dictionaries.find(lang2);

    if (d2 != dictionaries.end())
    {
      return d2->second.get();
    }
  }

  return nullptr;
}

} }
