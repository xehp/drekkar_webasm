;; To compile this do
;; sudo apt install binaryen
;; wasm-as test.wat
(module
  (func $test (param i32 i32) (result i32)
    
   (if (i32.add
      (local.get 0)
      (local.get 1)
    )
       (then
  	  (i32.const 7)  
       )
       (else
  	  (i32.const 11)
       )
     ) 
  )
  (export "test" (func $test))
)

