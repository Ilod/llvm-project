// RUN: %clang_cc1 %s -fblocks -triple x86_64-apple-darwin -emit-llvm -o - | FileCheck %s

namespace test0 {
  // CHECK: define void @_ZN5test04testEi(
  // CHECK: define internal void @__test_block_invoke_{{.*}}(
  // CHECK: define internal void @__block_global_{{.*}}(
  void test(int x) {
    ^{ ^{ (void) x; }; };
  }
}

extern void (^out)();

namespace test1 {
  // Capturing const objects doesn't require a local block.
  // CHECK: define void @_ZN5test15test1Ev()
  // CHECK:   store void ()* bitcast ({{.*}} @__block_literal_global{{.*}} to void ()*), void ()** @out
  void test1() {
    const int NumHorsemen = 4;
    out = ^{ (void) NumHorsemen; };
  }

  // That applies to structs too...
  // CHECK: define void @_ZN5test15test2Ev()
  // CHECK:   store void ()* bitcast ({{.*}} @__block_literal_global{{.*}} to void ()*), void ()** @out
  struct loc { double x, y; };
  void test2() {
    const loc target = { 5, 6 };
    out = ^{ (void) target; };
  }

  // ...unless they have mutable fields...
  // CHECK: define void @_ZN5test15test3Ev()
  // CHECK:   [[BLOCK:%.*]] = alloca [[BLOCK_T:<{.*}>]],
  // CHECK:   [[T0:%.*]] = bitcast [[BLOCK_T]]* [[BLOCK]] to void ()*
  // CHECK:   store void ()* [[T0]], void ()** @out
  struct mut { mutable int x; };
  void test3() {
    const mut obj = { 5 };
    out = ^{ (void) obj; };
  }

  // ...or non-trivial destructors...
  // CHECK: define void @_ZN5test15test4Ev()
  // CHECK:   [[OBJ:%.*]] = alloca
  // CHECK:   [[BLOCK:%.*]] = alloca [[BLOCK_T:<{.*}>]],
  // CHECK:   [[T0:%.*]] = bitcast [[BLOCK_T]]* [[BLOCK]] to void ()*
  // CHECK:   store void ()* [[T0]], void ()** @out
  struct scope { int x; ~scope(); };
  void test4() {
    const scope obj = { 5 };
    out = ^{ (void) obj; };
  }

  // ...or non-trivial copy constructors, but it's not clear how to do
  // that and still have a constant initializer in '03.
}

namespace test2 {
  struct A {
    A();
    A(const A &);
    ~A();
  };

  struct B {
    B();
    B(const B &);
    ~B();
  };

  // CHECK: define void @_ZN5test24testEv()
  void test() {
    __block A a;
    __block B b;
  }

  // CHECK: define internal void @__Block_byref_object_copy
  // CHECK: call void @_ZN5test21AC1ERKS0_(

  // CHECK: define internal void @__Block_byref_object_dispose
  // CHECK: call void @_ZN5test21AD1Ev(

  // CHECK: define internal void @__Block_byref_object_copy
  // CHECK: call void @_ZN5test21BC1ERKS0_(

  // CHECK: define internal void @__Block_byref_object_dispose
  // CHECK: call void @_ZN5test21BD1Ev(
}

// rdar://problem/9334739
// Make sure we mark destructors for parameters captured in blocks.
namespace test3 {
  struct A {
    A(const A&);
    ~A();
  };

  struct B : A {
  };

  void test(B b) {
    extern void consume(void(^)());
    consume(^{ (void) b; });
  }
}
