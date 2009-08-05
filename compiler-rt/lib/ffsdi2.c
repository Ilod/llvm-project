/* ===-- ffsdi2.c - Implement __ffsdi2 -------------------------------------===
 *
 *                     The LLVM Compiler Infrastructure
 *
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 *
 * This file implements __ffsdi2 for the compiler_rt library.
 *
 * ===----------------------------------------------------------------------===
 */

#include "int_lib.h"

/* Returns: the index of the least significant 1-bit in a, or
 * the value zero if a is zero. The least significant bit is index one.
 */

si_int
__ffsdi2(di_int a)
{
    dwords x;
    x.all = a;
    if (x.low == 0)
    {
        if (x.high == 0)
            return 0;
        return __builtin_ctz(x.high) + (1 + sizeof(si_int) * CHAR_BIT);
    }
    return __builtin_ctz(x.low) + 1;
}
