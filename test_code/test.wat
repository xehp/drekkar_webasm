;; To compile this do
;; sudo apt install binaryen
;; wasm-as test.wat
;; To get it back into text format do:
;; wasm-dis test.wasm
;; To run this:
;; ../drekkar_webasm_runtime/drekkar_webasm_runtime --function_name test ../test_code/test.wasm 4 -4
(module
  (import "drekkar" "log_i64" (func $log (param i32) ))
  (import "drekkar" "log_hex" (func $hex (param i32) ))
  (import "drekkar" "log_ch" (func $putc (param i32) ))
  (import "drekkar" "log_empty_line" (func $empty ))
  (import "drekkar" "log_str" (func $puts (param i32) ))
  (export "test" (func $test))
  (memory $0 1 1)
  (data (i32.const 1024) "hello, world\00")
  (data (i32.const 1044) "zero\00")
  (data (i32.const 1054) "not zero\00")
  (global $global$0 (mut i32) (i32.const 17))

  (func $test (param i32 i32) (result i32)  
    (call $empty)


    (if
      (i32.add
        (local.get 0)
        (local.get 1)
      )
      (then 
	    (call $puts     
		 (i32.const 1054)   
	       ) 
      )
      (else
	    (call $puts     
		 (i32.const 1044)   
	       ) 
      )
    )

    (call $empty)

    (if (i32.const 2)
       (then 
	    (call $log (i32.const 11))
	)
    )

   (call $hex (i32.const 1024))


    (call $puts     
	 (i32.const 1024)   
       ) 

    (call $empty)


    (i32.const 0)
  )

)
