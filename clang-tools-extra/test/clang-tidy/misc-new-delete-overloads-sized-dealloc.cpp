// RUN: %python %S/check_clang_tidy.py %s misc-new-delete-overloads %t -- -std=c++14 -fsized-deallocation

typedef decltype(sizeof(int)) size_t;

struct S {
  // CHECK-MESSAGES: :[[@LINE+1]]:8: warning: declaration of 'operator delete' has no matching declaration of 'operator new' at the same scope [misc-new-delete-overloads]
  void operator delete(void *ptr, size_t) noexcept; // not a placement delete
};

struct T {
  // Because we have enabled sized deallocations explicitly, this new/delete
  // pair matches.
  void *operator new(size_t size) noexcept;
  void operator delete(void *ptr, size_t) noexcept; // ok because sized deallocation is enabled
};

// While we're here, check that global operator delete with no operator new
// is also matched.
// CHECK-MESSAGES: :[[@LINE+1]]:6: warning: declaration of 'operator delete' has no matching declaration of 'operator new' at the same scope
void operator delete(void *ptr) noexcept;
