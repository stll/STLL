#ifndef __UTF_8_H__
#define __UTF_8_H__

#include <string>

bool u8_isValid(const std::string & str);
std::u32string u8_convertToU32(const std::string & in);

#endif