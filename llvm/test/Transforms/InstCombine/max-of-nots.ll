; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -S -instcombine < %s | FileCheck %s

define i32 @compute_min_2(i32 %x, i32 %y) {
; CHECK-LABEL: @compute_min_2(
; CHECK-NEXT:    [[TMP1:%.*]] = icmp slt i32 %x, %y
; CHECK-NEXT:    [[TMP2:%.*]] = select i1 [[TMP1]], i32 %x, i32 %y
; CHECK-NEXT:    ret i32 [[TMP2]]
;
  %not_x = sub i32 -1, %x
  %not_y = sub i32 -1, %y
  %cmp = icmp sgt i32 %not_x, %not_y
  %not_min = select i1 %cmp, i32 %not_x, i32 %not_y
  %min = sub i32 -1, %not_min
  ret i32 %min
}

define i32 @compute_min_3(i32 %x, i32 %y, i32 %z) {
; CHECK-LABEL: @compute_min_3(
; CHECK-NEXT:    [[TMP1:%.*]] = icmp slt i32 %x, %y
; CHECK-NEXT:    [[TMP2:%.*]] = select i1 [[TMP1]], i32 %x, i32 %y
; CHECK-NEXT:    [[TMP3:%.*]] = icmp slt i32 [[TMP2]], %z
; CHECK-NEXT:    [[TMP4:%.*]] = select i1 [[TMP3]], i32 [[TMP2]], i32 %z
; CHECK-NEXT:    ret i32 [[TMP4]]
;
  %not_x = sub i32 -1, %x
  %not_y = sub i32 -1, %y
  %not_z = sub i32 -1, %z
  %cmp_1 = icmp sgt i32 %not_x, %not_y
  %not_min_1 = select i1 %cmp_1, i32 %not_x, i32 %not_y
  %cmp_2 = icmp sgt i32 %not_min_1, %not_z
  %not_min_2 = select i1 %cmp_2, i32 %not_min_1, i32 %not_z
  %min = sub i32 -1, %not_min_2
  ret i32 %min
}

define i32 @compute_min_arithmetic(i32 %x, i32 %y) {
; CHECK-LABEL: @compute_min_arithmetic(
; CHECK-NEXT:    [[TMP1:%.*]] = add i32 %x, -4
; CHECK-NEXT:    [[TMP2:%.*]] = icmp slt i32 [[TMP1]], %y
; CHECK-NEXT:    [[TMP3:%.*]] = select i1 [[TMP2]], i32 [[TMP1]], i32 %y
; CHECK-NEXT:    [[TMP4:%.*]] = xor i32 [[TMP3]], -1
; CHECK-NEXT:    ret i32 [[TMP4]]
;
  %not_value = sub i32 3, %x
  %not_y = sub i32 -1, %y
  %cmp = icmp sgt i32 %not_value, %not_y
  %not_min = select i1 %cmp, i32 %not_value, i32 %not_y
  ret i32 %not_min
}

declare void @fake_use(i32)

define i32 @compute_min_pessimization(i32 %x, i32 %y) {
; CHECK-LABEL: @compute_min_pessimization(
; CHECK-NEXT:    [[NOT_VALUE:%.*]] = sub i32 3, %x
; CHECK-NEXT:    call void @fake_use(i32 [[NOT_VALUE]])
; CHECK-NEXT:    [[NOT_Y:%.*]] = xor i32 %y, -1
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i32 [[NOT_VALUE]], [[NOT_Y]]
; CHECK-NEXT:    [[NOT_MIN:%.*]] = select i1 [[CMP]], i32 [[NOT_VALUE]], i32 [[NOT_Y]]
; CHECK-NEXT:    [[MIN:%.*]] = xor i32 [[NOT_MIN]], -1
; CHECK-NEXT:    ret i32 [[MIN]]
;
  %not_value = sub i32 3, %x
  call void @fake_use(i32 %not_value)
  %not_y = sub i32 -1, %y
  %cmp = icmp sgt i32 %not_value, %not_y
  %not_min = select i1 %cmp, i32 %not_value, i32 %not_y
  %min = sub i32 -1, %not_min
  ret i32 %min
}

define i32 @max_of_nots(i32 %x, i32 %y) {
; CHECK-LABEL: @max_of_nots(
; CHECK-NEXT:    [[TMP1:%.*]] = icmp sgt i32 %y, 0
; CHECK-NEXT:    [[TMP2:%.*]] = select i1 [[TMP1]], i32 %y, i32 0
; CHECK-NEXT:    [[TMP3:%.*]] = icmp slt i32 [[TMP2]], %x
; CHECK-NEXT:    [[TMP4:%.*]] = select i1 [[TMP3]], i32 [[TMP2]], i32 %x
; CHECK-NEXT:    [[TMP5:%.*]] = xor i32 [[TMP4]], -1
; CHECK-NEXT:    ret i32 [[TMP5]]
;
  %c0 = icmp sgt i32 %y, 0
  %xor_y = xor i32 %y, -1
  %s0 = select i1 %c0, i32 %xor_y, i32 -1
  %xor_x = xor i32 %x, -1
  %c1 = icmp slt i32 %s0, %xor_x
  %smax96 = select i1 %c1, i32 %xor_x, i32 %s0
  ret i32 %smax96
}

define <2 x i32> @max_of_nots_vec(<2 x i32> %x, <2 x i32> %y) {
; CHECK-LABEL: @max_of_nots_vec(
; CHECK-NEXT:    [[TMP1:%.*]] = icmp sgt <2 x i32> %y, zeroinitializer
; CHECK-NEXT:    [[TMP2:%.*]] = select <2 x i1> [[TMP1]], <2 x i32> %y, <2 x i32> zeroinitializer
; CHECK-NEXT:    [[TMP3:%.*]] = icmp slt <2 x i32> [[TMP2]], %x
; CHECK-NEXT:    [[TMP4:%.*]] = select <2 x i1> [[TMP3]], <2 x i32> [[TMP2]], <2 x i32> %x
; CHECK-NEXT:    [[TMP5:%.*]] = xor <2 x i32> [[TMP4]], <i32 -1, i32 -1>
; CHECK-NEXT:    ret <2 x i32> [[TMP5]]
;
  %c0 = icmp sgt <2 x i32> %y, zeroinitializer
  %xor_y = xor <2 x i32> %y, <i32 -1, i32 -1>
  %s0 = select <2 x i1> %c0, <2 x i32> %xor_y, <2 x i32> <i32 -1, i32 -1>
  %xor_x = xor <2 x i32> %x, <i32 -1, i32 -1>
  %c1 = icmp slt <2 x i32> %s0, %xor_x
  %smax96 = select <2 x i1> %c1, <2 x i32> %xor_x, <2 x i32> %s0
  ret <2 x i32> %smax96
}

define <2 x i37> @max_of_nots_weird_type_vec(<2 x i37> %x, <2 x i37> %y) {
; CHECK-LABEL: @max_of_nots_weird_type_vec(
; CHECK-NEXT:    [[TMP1:%.*]] = icmp sgt <2 x i37> %y, zeroinitializer
; CHECK-NEXT:    [[TMP2:%.*]] = select <2 x i1> [[TMP1]], <2 x i37> %y, <2 x i37> zeroinitializer
; CHECK-NEXT:    [[TMP3:%.*]] = icmp slt <2 x i37> [[TMP2]], %x
; CHECK-NEXT:    [[TMP4:%.*]] = select <2 x i1> [[TMP3]], <2 x i37> [[TMP2]], <2 x i37> %x
; CHECK-NEXT:    [[TMP5:%.*]] = xor <2 x i37> [[TMP4]], <i37 -1, i37 -1>
; CHECK-NEXT:    ret <2 x i37> [[TMP5]]
;
  %c0 = icmp sgt <2 x i37> %y, zeroinitializer
  %xor_y = xor <2 x i37> %y, <i37 -1, i37 -1>
  %s0 = select <2 x i1> %c0, <2 x i37> %xor_y, <2 x i37> <i37 -1, i37 -1>
  %xor_x = xor <2 x i37> %x, <i37 -1, i37 -1>
  %c1 = icmp slt <2 x i37> %s0, %xor_x
  %smax96 = select <2 x i1> %c1, <2 x i37> %xor_x, <2 x i37> %s0
  ret <2 x i37> %smax96
}

