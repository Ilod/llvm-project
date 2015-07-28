// RUN: %clang_cc1 -fsyntax-only -Wredundant-move -std=c++11 -verify %s
// RUN: not %clang_cc1 -fsyntax-only -Wredundant-move -std=c++11 -fdiagnostics-parseable-fixits %s 2>&1 | FileCheck %s

// definitions for std::move
namespace std {
inline namespace foo {
template <class T> struct remove_reference { typedef T type; };
template <class T> struct remove_reference<T&> { typedef T type; };
template <class T> struct remove_reference<T&&> { typedef T type; };

template <class T> typename remove_reference<T>::type &&move(T &&t);
}
}

struct A {};
struct B : public A {};

A test1(B b1) {
  B b2;
  return b1;
  return b2;
  return std::move(b1);
  // expected-warning@-1{{redundant move}}
  // expected-note@-2{{remove std::move call}}
  // CHECK: fix-it:"{{.*}}":{[[@LINE-3]]:10-[[@LINE-3]]:20}:""
  // CHECK: fix-it:"{{.*}}":{[[@LINE-4]]:22-[[@LINE-4]]:23}:""
  return std::move(b2);
  // expected-warning@-1{{redundant move}}
  // expected-note@-2{{remove std::move call}}
  // CHECK: fix-it:"{{.*}}":{[[@LINE-3]]:10-[[@LINE-3]]:20}:""
  // CHECK: fix-it:"{{.*}}":{[[@LINE-4]]:22-[[@LINE-4]]:23}:""
}

struct C {
  C() {}
  C(A) {}
};

C test2(A a1, B b1) {
  A a2;
  B b2;

  return a1;
  return a2;
  return b1;
  return b2;

  return std::move(a1);
  // expected-warning@-1{{redundant move}}
  // expected-note@-2{{remove std::move call}}
  // CHECK: fix-it:"{{.*}}":{[[@LINE-3]]:10-[[@LINE-3]]:20}:""
  // CHECK: fix-it:"{{.*}}":{[[@LINE-4]]:22-[[@LINE-4]]:23}:""
  return std::move(a2);
  // expected-warning@-1{{redundant move}}
  // expected-note@-2{{remove std::move call}}
  // CHECK: fix-it:"{{.*}}":{[[@LINE-3]]:10-[[@LINE-3]]:20}:""
  // CHECK: fix-it:"{{.*}}":{[[@LINE-4]]:22-[[@LINE-4]]:23}:""
  return std::move(b1);
  // expected-warning@-1{{redundant move}}
  // expected-note@-2{{remove std::move call}}
  // CHECK: fix-it:"{{.*}}":{[[@LINE-3]]:10-[[@LINE-3]]:20}:""
  // CHECK: fix-it:"{{.*}}":{[[@LINE-4]]:22-[[@LINE-4]]:23}:""
  return std::move(b2);
  // expected-warning@-1{{redundant move}}
  // expected-note@-2{{remove std::move call}}
  // CHECK: fix-it:"{{.*}}":{[[@LINE-3]]:10-[[@LINE-3]]:20}:""
  // CHECK: fix-it:"{{.*}}":{[[@LINE-4]]:22-[[@LINE-4]]:23}:""
}

// Copy of tests above with types changed to reference types.
A test3(B& b1) {
  B& b2 = b1;
  return b1;
  return b2;
  return std::move(b1);
  return std::move(b2);
}

C test4(A& a1, B& b1) {
  A& a2 = a1;
  B& b2 = b1;

  return a1;
  return a2;
  return b1;
  return b2;

  return std::move(a1);
  return std::move(a2);
  return std::move(b1);
  return std::move(b2);
}

// PR23819, case 2
struct D {};
D test5(D d) {
  return d;

  return std::move(d);
  // expected-warning@-1{{redundant move in return statement}}
  // expected-note@-2{{remove std::move call here}}
  // CHECK: fix-it:"{{.*}}":{[[@LINE-3]]:10-[[@LINE-3]]:20}:""
  // CHECK: fix-it:"{{.*}}":{[[@LINE-4]]:21-[[@LINE-4]]:22}:""
}

// No more fix-its past here.
// CHECK-NOT: fix-it

// A deleted copy constructor will prevent moves without std::move
struct E {
  E(E &&e);
  E(const E &e) = delete;
  // expected-note@-1{{deleted here}}
};

struct F {
  F(E);
  // expected-note@-1{{passing argument to parameter here}}
};

F test6(E e) {
  return e;
  // expected-error@-1{{call to deleted constructor of 'E'}}
  return std::move(e);
}

struct G {
  G(G &&g);
  // expected-note@-1{{copy constructor is implicitly deleted because 'G' has a user-declared move constructor}}
};

struct H {
  H(G);
  // expected-note@-1{{passing argument to parameter here}}
};

H test6(G g) {
  return g;  // expected-error{{call to implicitly-deleted copy constructor of 'G'}}
  return std::move(g);
}
