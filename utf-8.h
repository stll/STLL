#ifndef __UTF_8_H__
#define __UTF_8_H__

/** \file
 *  \brief utf-8 helper code.
 */

#include <string>

/** Check, if a figen string is a valid utf-8 string
 *
 * \param str the string to Check
 * \return true, when utf-8 conform, false otherwise
 */
bool u8_isValid(const std::string & str);

/** Convert an utf-8 string to utf-32. No checking is done here, if the string
 * might come from an unsafe source, check it first
 *
 * \param in the string to convert
 * \return the utf-32 encoded string
 */
std::u32string u8_convertToU32(const std::string & in);

#endif