//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// test ratio_add

#include <ratio>

int main()
{
    {
    typedef std::ratio<1, 1> R1;
    typedef std::ratio<1, 1> R2;
    typedef std::ratio_add<R1, R2>::type R;
    static_assert(R::num == 2 && R::den == 1, "");
    }
    {
    typedef std::ratio<1, 2> R1;
    typedef std::ratio<1, 1> R2;
    typedef std::ratio_add<R1, R2>::type R;
    static_assert(R::num == 3 && R::den == 2, "");
    }
    {
    typedef std::ratio<-1, 2> R1;
    typedef std::ratio<1, 1> R2;
    typedef std::ratio_add<R1, R2>::type R;
    static_assert(R::num == 1 && R::den == 2, "");
    }
    {
    typedef std::ratio<1, -2> R1;
    typedef std::ratio<1, 1> R2;
    typedef std::ratio_add<R1, R2>::type R;
    static_assert(R::num == 1 && R::den == 2, "");
    }
    {
    typedef std::ratio<1, 2> R1;
    typedef std::ratio<-1, 1> R2;
    typedef std::ratio_add<R1, R2>::type R;
    static_assert(R::num == -1 && R::den == 2, "");
    }
    {
    typedef std::ratio<1, 2> R1;
    typedef std::ratio<1, -1> R2;
    typedef std::ratio_add<R1, R2>::type R;
    static_assert(R::num == -1 && R::den == 2, "");
    }
    {
    typedef std::ratio<56987354, 467584654> R1;
    typedef std::ratio<544668, 22145> R2;
    typedef std::ratio_add<R1, R2>::type R;
    static_assert(R::num == 127970191639601LL && R::den == 5177331081415LL, "");
    }
}
