; Gerado pelo compilador simples (.drg -> .asm)
ORG 0
    LDA CONST_0
    STA a
    LDA CONST_1
    STA b
    LDA a
    ADD b
    STA c
    HLT
CONST_0 DATA 10
CONST_1 DATA 20
a SPACE 1
b SPACE 1
c SPACE 1
