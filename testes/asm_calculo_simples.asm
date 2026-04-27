; Exemplo manual sem parser: calculo simples
; TESTE 1 - calculo simples (LDA + ADD + STA)
; Resultado esperado: RESULT = 15

ORG 0
    LDA VAL1
    ADD VAL2
    STA RESULT
    HLT

VAL1 DATA 10
VAL2 DATA 5
RESULT SPACE 1
