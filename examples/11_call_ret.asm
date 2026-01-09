; Simple CALL/RET Example
; Demonstrates subroutine calls and returns

; This program:
; 1. Calls a subroutine to add two numbers
; 2. Calls another subroutine to multiply by 2 (using shift)
; 3. Calls a nested subroutine example
; Result: R0 = ((5 + 3) * 2) + 1 = 17

START:
    ; Test 1: Simple function call
    LOADI R0, 5
    LOADI R1, 3
    CALL  ADD_NUMBERS    ; R0 = R0 + R1 = 8

    ; Test 2: Call multiply by 2
    CALL  MULTIPLY_BY_2  ; R0 = R0 * 2 = 16

    ; Test 3: Call increment (which uses nested call)
    CALL  INCREMENT      ; R0 = R0 + 1 = 17

    HALT

; ============================================================================
; ADD_NUMBERS - Add R1 to R0
; Inputs:  R0, R1
; Outputs: R0 = R0 + R1
; Modifies: R0 (flags)
; ============================================================================
ADD_NUMBERS:
    ADD   R0, R1
    RET

; ============================================================================
; MULTIPLY_BY_2 - Multiply R0 by 2 using left shift
; Inputs:  R0
; Outputs: R0 = R0 * 2
; Modifies: R0 (flags)
; ============================================================================
MULTIPLY_BY_2:
    SHL   R0
    RET

; ============================================================================
; INCREMENT - Increment R0 by calling ADD_ONE subroutine
; Inputs:  R0
; Outputs: R0 = R0 + 1
; Modifies: R0, R1 (flags)
; ============================================================================
INCREMENT:
    PUSH  R1             ; Preserve R1
    LOADI R1, 1
    CALL  ADD_ONE        ; Nested call!
    POP   R1             ; Restore R1
    RET

; ============================================================================
; ADD_ONE - Add 1 to R0 (helper for INCREMENT)
; Inputs:  R0, R1 (should be 1)
; Outputs: R0 = R0 + R1
; Modifies: R0 (flags)
; ============================================================================
ADD_ONE:
    ADD   R0, R1
    RET
