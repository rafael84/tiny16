loadi r0, 12
loadi r1, 10
cmp   r0, r1        ; compare r0 - r1, set Z and C flags
jz    equal         ; if Z=1: r0 == r1
jc    less          ; if C=1: r0 < r1 (borrow occurred)
                    ; otherwise: r0 > r1 (fall through)

greater:    loadi r2, 100
            jmp end

less:       loadi r2, 0
            jmp end

equal:      loadi r2, 50
            jmp end

end:        halt
