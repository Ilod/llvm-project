//===-- aeabi_drsub.c - Test __aeabi_drsub --------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file tests __aeabi_drsub for the compiler_rt library.
//
//===----------------------------------------------------------------------===//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#if __arm__
extern __attribute__((pcs("aapcs"))) double __aeabi_drsub(double a, double b);

int test__aeabi_drsub(double a, double b, double expected)
{
    double actual = __aeabi_drsub(a, b);
    if (actual != expected)
        printf("error in __aeabi_drsub(%f, %f) = %f, expected %f\n",
               a, b, actual, expected);
    return actual != expected;
}
#endif

int main()
{
#if __arm__
    if (test__aeabi_drsub(1.0, 1.0, 0.0))
        return 1;
    if (test__aeabi_drsub(1234.567, 765.4321, -469.134900))
        return 1;
    if (test__aeabi_drsub(-123.0, -678.0, -555.0))
        return 1;
    if (test__aeabi_drsub(0.0, -0.0, 0.0))
        return 1;
#else
    printf("skipped\n");
#endif
    return 0;
}
