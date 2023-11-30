;; To compile this do
;; sudo apt install binaryen
;; wasm-as test_const.wat
;; Expected result is sum of 3 numbers, one number is hardcoded.
;; Such as 11 + 13 - 1894007588 -> 2400959732 or 0x8f1bbcf4
(module
  (func $test (param i32 i32) (result i32)
    (i32.add
      (i32.const -1894007588)
      ;; 0x8F1BBCDC -1894007588
      ;; (i32.const 1000)
      (i32.add
        (local.get 0)
        (local.get 1)
      )
    )
  )
  (export "test" (func $test))
)
