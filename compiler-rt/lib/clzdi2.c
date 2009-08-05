/* ===-- clzdi2.c - Implement __clzdi2 -------------------------------------===
 *
 *      	       The LLVM Compiler Infrastructure
 *
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 *
 * This file implements __clzdi2 for the compiler_rt library.
 *
 * ===----------------------------------------------------------------------===
 */

#include "int_lib.h"

/* Returns: the number of leading 0-bits */

/* Precondition: a != 0 */

si_int
__clzdi2(di_int a)
{
    dwords x;
    x.all = a;
    const si_int f = -(x.high == 0);
    return __builtin_clz((x.high & ~f) | (x.low & f)) +
           (f & ((si_int)(sizeof(si_int) * CHAR_BIT)));
}
