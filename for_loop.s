.equ    I       1
.equ    N       2
.equ    RES     3

.data
n:                                      # // address of global variable N
        .byte   22

.text
        load    $n,     %N              #                  // load address n
        movzbq  (%N,    %0),    %N      #       N = *n;    // load value from n
        load    $0,     %RES            #       RES = 0;
        load    $1,     %I              #       I = 1;
        jmp     check                   #       goto check;
loop:                                   # loop:
        addq    %I,     %RES,   %RES    #       RES += I;
        addq    $1,     %I,     %I      #       ++I;
check:                                  # check:
        subq    %N,     %I,     %0      #       if (I-N>=0)
        jbe     loop                    #               goto loop;
        halt    %RES                    #       return RES
