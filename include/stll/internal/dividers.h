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

#ifndef __DIVIDERS_H__
#define __DIVIDERS_H__

namespace STLL { namespace internal {

template <class T>
T div_inf(T x, T y)
{
  T xdivy = x / y;
  T xmody = x % y;

  T tmp = (xmody && ((y ^ xmody) < 0)) ? 1 : 0;

  xdivy -= tmp;

  return xdivy;
}

template <class T>
T mod_inf(T x, T y)
{
  T xmody = x % y;

  T tmp = (xmody && ((y ^ xmody) < 0)) ? 1 : 0;

  xmody += tmp*y;

  return xmody;
}

template <class T>
auto divmod_inf(T x, T y)
{
  T xdivy = x / y;
  T xmody = x % y;

  T tmp = (xmody && ((y ^ xmody) < 0)) ? 1 : 0;

  xmody += tmp*y;
  xdivy -= tmp;

  return std::make_pair(xdivy, xmody);
}


#if 0 // the original python code
static enum divmod_result
i_divmod(register long x, register long y,
         long *p_xdivy, long *p_xmody)
{
  long xdivy, xmody;

  if (y == 0) {
    PyErr_SetString(PyExc_ZeroDivisionError,
                    "integer division or modulo by zero");
    return DIVMOD_ERROR;
  }
  /* (-sys.maxint-1)/-1 is the only overflow case. */
  if (y == -1 && UNARY_NEG_WOULD_OVERFLOW(x))
    return DIVMOD_OVERFLOW;
  xdivy = x / y;
  xmody = x - xdivy * y;
  /* If the signs of x and y differ, and the remainder is non-0,
   * C89 doesn't define whether xdivy is now the floor or the
   * ceiling of the infinitely precise quotient.  We want the floor,
   * and we have it iff the remainder's sign matches y's.
   */
  if (xmody && ((y ^ xmody) < 0) /* i.e. and signs differ */) {
    xmody += y;
    --xdivy;
    assert(xmody && ((y ^ xmody) >= 0));
  }
  *p_xdivy = xdivy;
  *p_xmody = xmody;
  return DIVMOD_OK;
}
#endif

} }

#endif
