;; Test program for drekkar webasm runtime.
;;
;; Some tools may be needed to run this:
;;   sudo apt install binaryen
;;
;; To compile this do:
;;   wasm-as test_if.wat
;;
;; To get it back into text format do:
;;   wasm-dis test_if.wasm
;;
;; To run this:
;; ../drekkar_webasm_runtime/drekkar_webasm_runtime --function_name test test_if.wasm -4 4
;;
;; Expected result:
;; If the sum of the two last numbers (in command line) is zero:
;;   log:
;;   log: 0
;;   log:
;;
;; If not zero:
;;   log:
;;   log: 1
;;   log:
;;
(module
  (import "console" "drekkar_log_i64" (func $log (param i32) ))
  (import "console" "drekkar_log_empty_line" (func $empty ))
  (export "test" (func $test))
  (global $global$0 (mut i32) (i32.const 17))

  (func $test (param i32 i32) (result i32)  
    (call $empty )

    (if
      (i32.add
        (local.get 0)
        (local.get 1)
      )
      (then 
	(call $log (i32.const 1))
      )
      (else
	(call $log (i32.const 0))
      )
    )

    (call $empty )

    (i32.const $0)
  )

)
