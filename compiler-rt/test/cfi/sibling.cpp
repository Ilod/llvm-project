// XFAIL: *

// RUN: %clangxx_cfi -o %t %s
// RUN: not --crash %t 2>&1 | FileCheck --check-prefix=CFI %s

// RUN: %clangxx_cfi -DB32 -o %t %s
// RUN: not --crash %t 2>&1 | FileCheck --check-prefix=CFI %s

// RUN: %clangxx_cfi -DB64 -o %t %s
// RUN: not --crash %t 2>&1 | FileCheck --check-prefix=CFI %s

// RUN: %clangxx_cfi -DBM -o %t %s
// RUN: not --crash %t 2>&1 | FileCheck --check-prefix=CFI %s

// RUN: %clangxx -o %t %s
// RUN: %t 2>&1 | FileCheck --check-prefix=NCFI %s

// Tests that the CFI enforcement distinguishes betwen non-overriding siblings.
// XFAILed as not implemented yet.

#include <stdio.h>
#include "utils.h"

struct A {
  virtual void f();
};

void A::f() {}

struct B : A {
  virtual void f();
};

void B::f() {}

struct C : A {
};

int main() {
#ifdef B32
  break_optimization(new Deriver<B, 0>);
#endif

#ifdef B64
  break_optimization(new Deriver<B, 0>);
  break_optimization(new Deriver<B, 1>);
#endif

#ifdef BM
  break_optimization(new Deriver<B, 0>);
  break_optimization(new Deriver<B, 1>);
  break_optimization(new Deriver<B, 2>);
#endif

  B *b = new B;
  break_optimization(b);

  // CFI: 1
  // NCFI: 1
  fprintf(stderr, "1\n");

  ((C *)b)->f(); // UB here

  // CFI-NOT: 2
  // NCFI: 2
  fprintf(stderr, "2\n");
}
