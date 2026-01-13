; Exercise 13: Find Element in Array
; Goal: Search for value 30 in array, return index (or 255 if not found)
; Array: [10, 20, 30, 40, 50] at 0x4000
; Instructions to use: LOADI, LOAD, SUB, JZ, INC, DEC
; Expected result: R0 = 2 (index where 30 is found)

section .code

; TODO: Your code here
; Strategy:
; - Use R6:R7 to point to array (0x4000)
; - Use R0 as index counter (starts at 0)
; - Use R1 as search value (30)
; - Use R2 to store array length (5)
; - Loop: LOAD element, compare with search value
;   - If equal: return current index
;   - If not equal: increment index and address, continue
; - If loop ends without finding: set R0 to 255

section .data

array: DB 10, 20, 30, 40, 50
search: DB 30

HALT
