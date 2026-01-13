; TIMES Pseudo-Instruction Test
;
; This example demonstrates the TIMES pseudo-instruction:
; - TIMES N instruction : repeats an instruction N times
; - Works in both code and data sections
;
; Example use cases:
; - Shift operations: TIMES 4 SHR R0  (divide by 16)
; - Buffer allocation: TIMES 64 DB 0  (64 zero bytes)
;

section .code

START:
    ; Test 1: Use TIMES with shift operations
    ; Load 128 and shift right 4 times = 128 / 16 = 8
    LOADI R0, 128         ; R0 = 128 (0x80)
    TIMES 4 SHR R0        ; R0 = 128 >> 4 = 8

    ; Verify result: R0 should be 8
    LOADI R1, 8
    CMP   R0, R1
    JNZ   FAIL

    ; Test 2: Use TIMES with shift left
    ; Load 1 and shift left 5 times = 1 << 5 = 32
    LOADI R0, 1           ; R0 = 1
    TIMES 5 SHL R0        ; R0 = 1 << 5 = 32

    ; Verify result: R0 should be 32
    LOADI R1, 32
    CMP   R0, R1
    JNZ   FAIL

    ; Test 3: Use TIMES with INC
    ; Start at 0 and increment 10 times
    LOADI R0, 0           ; R0 = 0
    TIMES 10 INC R0       ; R0 = 10

    ; Verify result: R0 should be 10
    LOADI R1, 10
    CMP   R0, R1
    JNZ   FAIL

    ; Test 4: Verify data section TIMES worked
    ; Load byte from buffer (should be 0xAA)
    LOADI R6, 0x40        ; High byte of data address
    LOADI R7, 0x40        ; pattern starts at 0x4040
    LOAD  R0
    LOADI R1, 0xAA
    CMP   R0, R1
    JNZ   FAIL

    ; All tests passed! Store success marker
    LOADI R6, 0x40
    LOADI R7, 0x00
    LOADI R0, 0x42        ; 'B' for "good"
    STORE R0
    HALT

FAIL:
    ; Test failed - store failure marker
    LOADI R6, 0x40
    LOADI R7, 0x00
    LOADI R0, 0x46        ; 'F' for fail
    STORE R0
    HALT

section .data

; Result storage at 0x4000
result:       DB 0

; Buffer initialized with zeros using TIMES (63 bytes padding)
buffer:       TIMES 63 DB 0

; Pattern at 0x4040: 16 bytes of 0xAA
pattern:      TIMES 16 DB 0xAA

; Another pattern: 8 bytes of 0x55
pattern2:     TIMES 8 DB 0x55

; Mixed: repeated pattern of two bytes
; Creates: FF 00 FF 00 FF 00 FF 00
mixed:        TIMES 4 DB 0xFF, 0x00
