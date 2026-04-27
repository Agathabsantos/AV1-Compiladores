; Gerado pelo compilador simples (.drg -> .asm)
ORG 0
    LDA CONST_0
    STA a
    LDA CONST_1
    STA b
    LDA a
    JN FIM_IF_0
    JZ FIM_IF_0
    LDA a
    ADD b
    STA b
FIM_IF_0:
INI_WHILE_1:
    LDA a
    JN FIM_WHILE_1
    JZ FIM_WHILE_1
    LDA a
    ADD CONST_2
    STA a
    JMP INI_WHILE_1
FIM_WHILE_1:
    HLT
CONST_0 DATA 5
CONST_1 DATA 0
CONST_2 DATA -1
a SPACE 1
b SPACE 1
