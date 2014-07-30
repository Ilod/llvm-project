; RUN: opt < %s -constprop | llvm-dis -disable-output
; RUN: verify-uselistorder < %s -preserve-bc-use-list-order -num-shuffles=5
; PR3465

define double @test() {
  %tmp24 = extractelement <2 x double> bitcast (<1 x i128> < i128 85070591730234615870450834276742070272 > to <2 x double>), i32 0
  ret double %tmp24
}

