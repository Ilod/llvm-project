// The line and column layout of this test is significant. Run lines
// are at the end.
typedef void (^block_t)(float f, double d);
void f(int (^block)(int x, int y));
void g(block_t b);

void test_f() {

}

@interface A
- method:(int (^)(int x, int y))b;
- method2:(block_t)b;
@end

void test_A(A *a) {
  [a method:0];
}

@interface B
- method3:(int (^)(void))b;
@end

void test_B(B *b) {
  [b method3:^int(void){ return 0; }];
}

@interface C
- method4:(void(^)(void))arg { };
@end

void test_C(C *c) {
  [c method4:^{}];
}

// RUN: c-index-test -code-completion-at=%s:8:1 %s | FileCheck -check-prefix=CHECK-CC1 %s
// CHECK-CC1: FunctionDecl:{ResultType void}{TypedText f}{LeftParen (}{Placeholder ^int(int x, int y)block}{RightParen )} (50)
// CHECK-CC1: FunctionDecl:{ResultType void}{TypedText g}{LeftParen (}{Placeholder ^(float f, double d)b}{RightParen )} (50)
// RUN: c-index-test -code-completion-at=%s:17:6 %s | FileCheck -check-prefix=CHECK-CC2 %s
// CHECK-CC2: ObjCInstanceMethodDecl:{ResultType id}{TypedText method2:}{Placeholder ^(float f, double d)b} (20)
// CHECK-CC2: ObjCInstanceMethodDecl:{ResultType id}{TypedText method:}{Placeholder ^int(int x, int y)b} (20)
// RUN: c-index-test -code-completion-at=%s:25:6 %s | FileCheck -check-prefix=CHECK-CC3 %s
// CHECK-CC3: ObjCInstanceMethodDecl:{ResultType id}{TypedText method3:}{Placeholder ^int(void)b} (20)
// RUN: c-index-test -code-completion-at=%s:33:6 %s | FileCheck -check-prefix=CHECK-CC4 %s
// CHECK-CC4: ObjCInstanceMethodDecl:{ResultType id}{TypedText method4:}{Placeholder ^(void)arg} (20)
