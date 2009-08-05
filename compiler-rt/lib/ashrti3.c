/* ===-- ashrti3.c - Implement __ashrti3 -----------------------------------===
 *
 *                     The LLVM Compiler Infrastructure
 *
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 *
 * This file implements __ashrti3 for the compiler_rt library.
 *
 * ===----------------------------------------------------------------------===
 */

#if __x86_64

#include "int_lib.h"

/* Returns: arithmetic a >> b */

/* Precondition:  0 <= b < bits_in_tword */

ti_int
__ashrti3(ti_int a, si_int b)
{
    const int bits_in_dword = (int)(sizeof(di_int) * CHAR_BIT);
    twords input;
    twords result;
    input.all = a;
    if (b & bits_in_dword)  /* bits_in_dword <= b < bits_in_tword */
    {
        /* result.high = input.high < 0 ? -1 : 0 */
        result.high = input.high >> (bits_in_dword - 1);
        result.low = input.high >> (b - bits_in_dword);
    }
    else  /* 0 <= b < bits_in_dword */
    {
        if (b == 0)
            return a;
        result.high  = input.high >> b;
        result.low = (input.high << (bits_in_dword - b)) | (input.low >> b);
    }
    return result.all;
}

#endif
