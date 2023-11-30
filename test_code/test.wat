;; To compile this do
;; sudo apt install binaryen
;; wasm-as test.wat
;; To get it back into text format do:
;; wasm-dis test.wasm
;; To run this:
;; ./drekkar_webasm_runtime --logging-on --push-numbers ../test_code/test.wasm 0 0
(module
  (import "console" "test_log" (func $log (param i32) ))
  (import "console" "test_hello" (func $hello ))
  (export "test" (func $test))
  (global $global$0 (mut i32) (i32.const 17))

  (func $test (param i32 i32) (result i32)  
    (call $hello )

    (if (i32.const 0)
       (then 
	    (call $log (i32.const 7))
	)
	(else
	    (call $log (i32.const 9))
	)
    )

    (call $hello )

    (if (i32.const 2)
       (then 
	    (call $log (i32.const 11))
	)
    )

    (i32.add
      (local.get 0)
      (local.get 1)
    )
  )

)
