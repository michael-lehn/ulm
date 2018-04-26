.equ    N       5                       # Immediate value for vector length
.equ    I       1                       # register for i
.equ    X       2                       # address of x
.equ    RES     3                       # register for res
.equ    TMP     4                       # register for tmp

.text
        load    $0,     %I              # I = 0
        load    $x,     %X              # address of x[0]
        load    $0,     %RES            # result = 0
        jmp     check                   # goto check
loop:
        movzbq  (%X,    %I),    %TMP    # x[I]
        addq    %TMP,   %RES,   %RES    # result += x[I]
        addq    $1,     %I,     %I      # ++I
check:
        subq    $N,     %I,     %0      # If (I<N)
        jb      loop                    #       goto loop
        halt    %RES                    # return result
.data
x:
        .byte   1
        .byte   2
        .byte   3
        .byte   4
        .byte   5
