; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=i686-apple-darwin9 -mattr=+sse,+sse2,+sse4.1 | FileCheck %s --check-prefix=X32
; RUN: llc < %s -mtriple=x86_64-apple-darwin9 -mattr=+sse,+sse2,+sse4.1 | FileCheck %s --check-prefix=X64

define i16 @test1(float %f) nounwind {
; X32-LABEL: test1:
; X32:       ## BB#0:
; X32-NEXT:    movss {{.*#+}} xmm0 = mem[0],zero,zero,zero
; X32-NEXT:    xorps %xmm1, %xmm1
; X32-NEXT:    subss LCPI0_0, %xmm0
; X32-NEXT:    mulss LCPI0_1, %xmm0
; X32-NEXT:    minss LCPI0_2, %xmm0
; X32-NEXT:    maxss %xmm1, %xmm0
; X32-NEXT:    cvttss2si %xmm0, %eax
; X32-NEXT:    ## kill: %AX<def> %AX<kill> %EAX<kill>
; X32-NEXT:    retl
;
; X64-LABEL: test1:
; X64:       ## BB#0:
; X64-NEXT:    xorps %xmm1, %xmm1
; X64-NEXT:    blendps {{.*#+}} xmm0 = xmm0[0],xmm1[1,2,3]
; X64-NEXT:    subss {{.*}}(%rip), %xmm0
; X64-NEXT:    mulss {{.*}}(%rip), %xmm0
; X64-NEXT:    minss {{.*}}(%rip), %xmm0
; X64-NEXT:    maxss %xmm1, %xmm0
; X64-NEXT:    cvttss2si %xmm0, %eax
; X64-NEXT:    ## kill: %AX<def> %AX<kill> %EAX<kill>
; X64-NEXT:    retq
  %tmp = insertelement <4 x float> undef, float %f, i32 0		; <<4 x float>> [#uses=1]
  %tmp10 = insertelement <4 x float> %tmp, float 0.000000e+00, i32 1		; <<4 x float>> [#uses=1]
  %tmp11 = insertelement <4 x float> %tmp10, float 0.000000e+00, i32 2		; <<4 x float>> [#uses=1]
  %tmp12 = insertelement <4 x float> %tmp11, float 0.000000e+00, i32 3		; <<4 x float>> [#uses=1]
  %tmp28 = tail call <4 x float> @llvm.x86.sse.sub.ss( <4 x float> %tmp12, <4 x float> < float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00 > )		; <<4 x float>> [#uses=1]
  %tmp37 = tail call <4 x float> @llvm.x86.sse.mul.ss( <4 x float> %tmp28, <4 x float> < float 5.000000e-01, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00 > )		; <<4 x float>> [#uses=1]
  %tmp48 = tail call <4 x float> @llvm.x86.sse.min.ss( <4 x float> %tmp37, <4 x float> < float 6.553500e+04, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00 > )		; <<4 x float>> [#uses=1]
  %tmp59 = tail call <4 x float> @llvm.x86.sse.max.ss( <4 x float> %tmp48, <4 x float> zeroinitializer )		; <<4 x float>> [#uses=1]
  %tmp.upgrd.1 = tail call i32 @llvm.x86.sse.cvttss2si( <4 x float> %tmp59 )		; <i32> [#uses=1]
  %tmp69 = trunc i32 %tmp.upgrd.1 to i16		; <i16> [#uses=1]
  ret i16 %tmp69
}

define i16 @test2(float %f) nounwind {
; X32-LABEL: test2:
; X32:       ## BB#0:
; X32-NEXT:    movss {{.*#+}} xmm0 = mem[0],zero,zero,zero
; X32-NEXT:    addss LCPI1_0, %xmm0
; X32-NEXT:    mulss LCPI1_1, %xmm0
; X32-NEXT:    minss LCPI1_2, %xmm0
; X32-NEXT:    xorps %xmm1, %xmm1
; X32-NEXT:    maxss %xmm1, %xmm0
; X32-NEXT:    cvttss2si %xmm0, %eax
; X32-NEXT:    ## kill: %AX<def> %AX<kill> %EAX<kill>
; X32-NEXT:    retl
;
; X64-LABEL: test2:
; X64:       ## BB#0:
; X64-NEXT:    addss {{.*}}(%rip), %xmm0
; X64-NEXT:    mulss {{.*}}(%rip), %xmm0
; X64-NEXT:    minss {{.*}}(%rip), %xmm0
; X64-NEXT:    xorps %xmm1, %xmm1
; X64-NEXT:    maxss %xmm1, %xmm0
; X64-NEXT:    cvttss2si %xmm0, %eax
; X64-NEXT:    ## kill: %AX<def> %AX<kill> %EAX<kill>
; X64-NEXT:    retq
  %tmp28 = fsub float %f, 1.000000e+00		; <float> [#uses=1]
  %tmp37 = fmul float %tmp28, 5.000000e-01		; <float> [#uses=1]
  %tmp375 = insertelement <4 x float> undef, float %tmp37, i32 0		; <<4 x float>> [#uses=1]
  %tmp48 = tail call <4 x float> @llvm.x86.sse.min.ss( <4 x float> %tmp375, <4 x float> < float 6.553500e+04, float undef, float undef, float undef > )		; <<4 x float>> [#uses=1]
  %tmp59 = tail call <4 x float> @llvm.x86.sse.max.ss( <4 x float> %tmp48, <4 x float> < float 0.000000e+00, float undef, float undef, float undef > )		; <<4 x float>> [#uses=1]
  %tmp = tail call i32 @llvm.x86.sse.cvttss2si( <4 x float> %tmp59 )		; <i32> [#uses=1]
  %tmp69 = trunc i32 %tmp to i16		; <i16> [#uses=1]
  ret i16 %tmp69
}

declare <4 x float> @llvm.x86.sse.sub.ss(<4 x float>, <4 x float>)

declare <4 x float> @llvm.x86.sse.mul.ss(<4 x float>, <4 x float>)

declare <4 x float> @llvm.x86.sse.min.ss(<4 x float>, <4 x float>)

declare <4 x float> @llvm.x86.sse.max.ss(<4 x float>, <4 x float>)

declare i32 @llvm.x86.sse.cvttss2si(<4 x float>)

declare <4 x float> @llvm.x86.sse41.round.ss(<4 x float>, <4 x float>, i32)

declare <4 x float> @f()

define <4 x float> @test3(<4 x float> %A, float *%b, i32 %C) nounwind {
; X32-LABEL: test3:
; X32:       ## BB#0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    roundss $4, (%eax), %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: test3:
; X64:       ## BB#0:
; X64-NEXT:    roundss $4, (%rdi), %xmm0
; X64-NEXT:    retq
  %a = load float , float *%b
  %B = insertelement <4 x float> undef, float %a, i32 0
  %X = call <4 x float> @llvm.x86.sse41.round.ss(<4 x float> %A, <4 x float> %B, i32 4)
  ret <4 x float> %X
}

define <4 x float> @test4(<4 x float> %A, float *%b, i32 %C) nounwind {
; X32-LABEL: test4:
; X32:       ## BB#0:
; X32-NEXT:    subl $28, %esp
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movss {{.*#+}} xmm0 = mem[0],zero,zero,zero
; X32-NEXT:    movaps %xmm0, (%esp) ## 16-byte Spill
; X32-NEXT:    calll _f
; X32-NEXT:    movaps (%esp), %xmm1 ## 16-byte Reload
; X32-NEXT:    roundss $4, %xmm1, %xmm0
; X32-NEXT:    addl $28, %esp
; X32-NEXT:    retl
;
; X64-LABEL: test4:
; X64:       ## BB#0:
; X64-NEXT:    subq $24, %rsp
; X64-NEXT:    movss {{.*#+}} xmm0 = mem[0],zero,zero,zero
; X64-NEXT:    movaps %xmm0, (%rsp) ## 16-byte Spill
; X64-NEXT:    callq _f
; X64-NEXT:    movaps (%rsp), %xmm1 ## 16-byte Reload
; X64-NEXT:    roundss $4, %xmm1, %xmm0
; X64-NEXT:    addq $24, %rsp
; X64-NEXT:    retq
  %a = load float , float *%b
  %B = insertelement <4 x float> undef, float %a, i32 0
  %q = call <4 x float> @f()
  %X = call <4 x float> @llvm.x86.sse41.round.ss(<4 x float> %q, <4 x float> %B, i32 4)
  ret <4 x float> %X
}

; PR13576
define  <2 x double> @test5() nounwind uwtable readnone noinline {
; X32-LABEL: test5:
; X32:       ## BB#0: ## %entry
; X32-NEXT:    movaps {{.*#+}} xmm0 = [4.569870e+02,1.233210e+02]
; X32-NEXT:    movl $128, %eax
; X32-NEXT:    cvtsi2sdl %eax, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: test5:
; X64:       ## BB#0: ## %entry
; X64-NEXT:    movaps {{.*#+}} xmm0 = [4.569870e+02,1.233210e+02]
; X64-NEXT:    movl $128, %eax
; X64-NEXT:    cvtsi2sdl %eax, %xmm0
; X64-NEXT:    retq
entry:
  %0 = tail call <2 x double> @llvm.x86.sse2.cvtsi2sd(<2 x double> <double 4.569870e+02, double 1.233210e+02>, i32 128) nounwind readnone
  ret <2 x double> %0
}

declare <2 x double> @llvm.x86.sse2.cvtsi2sd(<2 x double>, i32) nounwind readnone

define <4 x float> @minss_fold(float* %x, <4 x float> %y) {
; X32-LABEL: minss_fold:
; X32:       ## BB#0: ## %entry
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    minss (%eax), %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: minss_fold:
; X64:       ## BB#0: ## %entry
; X64-NEXT:    minss (%rdi), %xmm0
; X64-NEXT:    retq
entry:
  %0 = load float, float* %x, align 1
  %vecinit.i = insertelement <4 x float> undef, float %0, i32 0
  %vecinit2.i = insertelement <4 x float> %vecinit.i, float 0.000000e+00, i32 1
  %vecinit3.i = insertelement <4 x float> %vecinit2.i, float 0.000000e+00, i32 2
  %vecinit4.i = insertelement <4 x float> %vecinit3.i, float 0.000000e+00, i32 3
  %1 = tail call <4 x float> @llvm.x86.sse.min.ss(<4 x float> %y, <4 x float> %vecinit4.i)
  ret <4 x float> %1
}

define <4 x float> @maxss_fold(float* %x, <4 x float> %y) {
; X32-LABEL: maxss_fold:
; X32:       ## BB#0: ## %entry
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    maxss (%eax), %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: maxss_fold:
; X64:       ## BB#0: ## %entry
; X64-NEXT:    maxss (%rdi), %xmm0
; X64-NEXT:    retq
entry:
  %0 = load float, float* %x, align 1
  %vecinit.i = insertelement <4 x float> undef, float %0, i32 0
  %vecinit2.i = insertelement <4 x float> %vecinit.i, float 0.000000e+00, i32 1
  %vecinit3.i = insertelement <4 x float> %vecinit2.i, float 0.000000e+00, i32 2
  %vecinit4.i = insertelement <4 x float> %vecinit3.i, float 0.000000e+00, i32 3
  %1 = tail call <4 x float> @llvm.x86.sse.max.ss(<4 x float> %y, <4 x float> %vecinit4.i)
  ret <4 x float> %1
}

define <4 x float> @cmpss_fold(float* %x, <4 x float> %y) {
; X32-LABEL: cmpss_fold:
; X32:       ## BB#0: ## %entry
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    cmpeqss (%eax), %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: cmpss_fold:
; X64:       ## BB#0: ## %entry
; X64-NEXT:    cmpeqss (%rdi), %xmm0
; X64-NEXT:    retq
entry:
  %0 = load float, float* %x, align 1
  %vecinit.i = insertelement <4 x float> undef, float %0, i32 0
  %vecinit2.i = insertelement <4 x float> %vecinit.i, float 0.000000e+00, i32 1
  %vecinit3.i = insertelement <4 x float> %vecinit2.i, float 0.000000e+00, i32 2
  %vecinit4.i = insertelement <4 x float> %vecinit3.i, float 0.000000e+00, i32 3
  %1 = tail call <4 x float> @llvm.x86.sse.cmp.ss(<4 x float> %y, <4 x float> %vecinit4.i, i8 0)
  ret <4 x float> %1
}
declare <4 x float> @llvm.x86.sse.cmp.ss(<4 x float>, <4 x float>, i8) nounwind readnone
