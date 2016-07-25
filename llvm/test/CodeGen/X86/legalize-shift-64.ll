; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=i686-unknown-unknown | FileCheck %s

define i64 @test1(i32 %xx, i32 %test) nounwind {
; CHECK-LABEL: test1:
; CHECK:       # BB#0:
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    andb $7, %cl
; CHECK-NEXT:    movl %edx, %eax
; CHECK-NEXT:    shll %cl, %eax
; CHECK-NEXT:    shrl %edx
; CHECK-NEXT:    xorb $31, %cl
; CHECK-NEXT:    shrl %cl, %edx
; CHECK-NEXT:    retl
  %conv = zext i32 %xx to i64
  %and = and i32 %test, 7
  %sh_prom = zext i32 %and to i64
  %shl = shl i64 %conv, %sh_prom
  ret i64 %shl
}

define i64 @test2(i64 %xx, i32 %test) nounwind {
; CHECK-LABEL: test2:
; CHECK:       # BB#0:
; CHECK-NEXT:    pushl %esi
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %esi
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %ch
; CHECK-NEXT:    andb $7, %ch
; CHECK-NEXT:    movb %ch, %cl
; CHECK-NEXT:    shll %cl, %esi
; CHECK-NEXT:    movl %eax, %edx
; CHECK-NEXT:    shrl %edx
; CHECK-NEXT:    xorb $31, %cl
; CHECK-NEXT:    shrl %cl, %edx
; CHECK-NEXT:    orl %esi, %edx
; CHECK-NEXT:    movb %ch, %cl
; CHECK-NEXT:    shll %cl, %eax
; CHECK-NEXT:    popl %esi
; CHECK-NEXT:    retl
  %and = and i32 %test, 7
  %sh_prom = zext i32 %and to i64
  %shl = shl i64 %xx, %sh_prom
  ret i64 %shl
}

define i64 @test3(i64 %xx, i32 %test) nounwind {
; CHECK-LABEL: test3:
; CHECK:       # BB#0:
; CHECK-NEXT:    pushl %esi
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %esi
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %ch
; CHECK-NEXT:    andb $7, %ch
; CHECK-NEXT:    movb %ch, %cl
; CHECK-NEXT:    shrl %cl, %esi
; CHECK-NEXT:    leal (%edx,%edx), %eax
; CHECK-NEXT:    xorb $31, %cl
; CHECK-NEXT:    shll %cl, %eax
; CHECK-NEXT:    orl %esi, %eax
; CHECK-NEXT:    movb %ch, %cl
; CHECK-NEXT:    shrl %cl, %edx
; CHECK-NEXT:    popl %esi
; CHECK-NEXT:    retl
  %and = and i32 %test, 7
  %sh_prom = zext i32 %and to i64
  %shr = lshr i64 %xx, %sh_prom
  ret i64 %shr
}

define i64 @test4(i64 %xx, i32 %test) nounwind {
; CHECK-LABEL: test4:
; CHECK:       # BB#0:
; CHECK-NEXT:    pushl %esi
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %esi
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %ch
; CHECK-NEXT:    andb $7, %ch
; CHECK-NEXT:    movb %ch, %cl
; CHECK-NEXT:    shrl %cl, %esi
; CHECK-NEXT:    leal (%edx,%edx), %eax
; CHECK-NEXT:    xorb $31, %cl
; CHECK-NEXT:    shll %cl, %eax
; CHECK-NEXT:    orl %esi, %eax
; CHECK-NEXT:    movb %ch, %cl
; CHECK-NEXT:    sarl %cl, %edx
; CHECK-NEXT:    popl %esi
; CHECK-NEXT:    retl
  %and = and i32 %test, 7
  %sh_prom = zext i32 %and to i64
  %shr = ashr i64 %xx, %sh_prom
  ret i64 %shr
}

; PR14668
define <2 x i64> @test5(<2 x i64> %A, <2 x i64> %B) {
; CHECK-LABEL: test5:
; CHECK:       # BB#0:
; CHECK-NEXT:    pushl %ebp
; CHECK-NEXT:  .Ltmp0:
; CHECK-NEXT:    .cfi_def_cfa_offset 8
; CHECK-NEXT:    pushl %ebx
; CHECK-NEXT:  .Ltmp1:
; CHECK-NEXT:    .cfi_def_cfa_offset 12
; CHECK-NEXT:    pushl %edi
; CHECK-NEXT:  .Ltmp2:
; CHECK-NEXT:    .cfi_def_cfa_offset 16
; CHECK-NEXT:    pushl %esi
; CHECK-NEXT:  .Ltmp3:
; CHECK-NEXT:    .cfi_def_cfa_offset 20
; CHECK-NEXT:  .Ltmp4:
; CHECK-NEXT:    .cfi_offset %esi, -20
; CHECK-NEXT:  .Ltmp5:
; CHECK-NEXT:    .cfi_offset %edi, -16
; CHECK-NEXT:  .Ltmp6:
; CHECK-NEXT:    .cfi_offset %ebx, -12
; CHECK-NEXT:  .Ltmp7:
; CHECK-NEXT:    .cfi_offset %ebp, -8
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %ebx
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %esi
; CHECK-NEXT:    movl %ebx, %edi
; CHECK-NEXT:    shll %cl, %edi
; CHECK-NEXT:    shldl %cl, %ebx, %esi
; CHECK-NEXT:    testb $32, %cl
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %ebp
; CHECK-NEXT:    je .LBB4_2
; CHECK-NEXT:  # BB#1:
; CHECK-NEXT:    movl %edi, %esi
; CHECK-NEXT:    xorl %edi, %edi
; CHECK-NEXT:  .LBB4_2:
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movl %edx, %ebx
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    shll %cl, %ebx
; CHECK-NEXT:    shldl %cl, %edx, %ebp
; CHECK-NEXT:    testb $32, %cl
; CHECK-NEXT:    je .LBB4_4
; CHECK-NEXT:  # BB#3:
; CHECK-NEXT:    movl %ebx, %ebp
; CHECK-NEXT:    xorl %ebx, %ebx
; CHECK-NEXT:  .LBB4_4:
; CHECK-NEXT:    movl %ebp, 12(%eax)
; CHECK-NEXT:    movl %ebx, 8(%eax)
; CHECK-NEXT:    movl %esi, 4(%eax)
; CHECK-NEXT:    movl %edi, (%eax)
; CHECK-NEXT:    popl %esi
; CHECK-NEXT:    popl %edi
; CHECK-NEXT:    popl %ebx
; CHECK-NEXT:    popl %ebp
; CHECK-NEXT:    retl $4
  %shl = shl <2 x i64> %A, %B
  ret <2 x i64> %shl
}

; PR16108
define i32 @test6() {
; CHECK-LABEL: test6:
; CHECK:       # BB#0:
; CHECK-NEXT:    pushl %ebp
; CHECK-NEXT:  .Ltmp8:
; CHECK-NEXT:    .cfi_def_cfa_offset 8
; CHECK-NEXT:  .Ltmp9:
; CHECK-NEXT:    .cfi_offset %ebp, -8
; CHECK-NEXT:    movl %esp, %ebp
; CHECK-NEXT:  .Ltmp10:
; CHECK-NEXT:    .cfi_def_cfa_register %ebp
; CHECK-NEXT:    andl $-8, %esp
; CHECK-NEXT:    subl $16, %esp
; CHECK-NEXT:    movl $1, {{[0-9]+}}(%esp)
; CHECK-NEXT:    movl $0, {{[0-9]+}}(%esp)
; CHECK-NEXT:    movl $1, (%esp)
; CHECK-NEXT:    movl $1, %eax
; CHECK-NEXT:    xorl %ecx, %ecx
; CHECK-NEXT:    shldl $32, %eax, %ecx
; CHECK-NEXT:    movb $32, %dl
; CHECK-NEXT:    testb %dl, %dl
; CHECK-NEXT:    jne .LBB5_2
; CHECK-NEXT:  # BB#1:
; CHECK-NEXT:    movl %ecx, %eax
; CHECK-NEXT:  .LBB5_2:
; CHECK-NEXT:    sete %cl
; CHECK-NEXT:    movzbl %cl, %ecx
; CHECK-NEXT:    xorl $1, %eax
; CHECK-NEXT:    orl %ecx, %eax
; CHECK-NEXT:    je .LBB5_5
; CHECK-NEXT:  # BB#3: # %if.then
; CHECK-NEXT:    movl $1, %eax
; CHECK-NEXT:    jmp .LBB5_4
; CHECK-NEXT:  .LBB5_5: # %if.end
; CHECK-NEXT:    xorl %eax, %eax
; CHECK-NEXT:  .LBB5_4: # %if.then
; CHECK-NEXT:    movl %ebp, %esp
; CHECK-NEXT:    popl %ebp
; CHECK-NEXT:    retl
  %x = alloca i32, align 4
  %t = alloca i64, align 8
  store i32 1, i32* %x, align 4
  store i64 1, i64* %t, align 8  ;; DEAD
  %load = load i32, i32* %x, align 4
  %shl = shl i32 %load, 8
  %add = add i32 %shl, -224
  %sh_prom = zext i32 %add to i64
  %shl1 = shl i64 1, %sh_prom
  %cmp = icmp ne i64 %shl1, 4294967296
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 1

if.end:                                           ; preds = %entry
  ret i32 0

}
