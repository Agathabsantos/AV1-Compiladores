; Exemplo manual sem parser: condicional 
; TESTE 2 - estrutura condicional (if) com JMP, JN e JZ
; Regra:
;   se AC == 0 -> RESULT = 10
;   se AC < 0  -> RESULT = 20
;   senao      -> RESULT = 30
; Resultado esperado (com VALOR = 0): RESULT = 10

ORG 0
    LDA VALOR
    JN EH_NEGATIVO
    JZ EH_ZERO
    JMP EH_POSITIVO

EH_NEGATIVO:
    LDA COD_NEG
    STA RESULT
    HLT

EH_ZERO:
    LDA COD_ZERO
    STA RESULT
    JMP FIM

EH_POSITIVO:
    LDA COD_POS
    STA RESULT

FIM:
    HLT

VALOR DATA 0
COD_ZERO DATA 10
COD_NEG DATA 20
COD_POS DATA 30
RESULT SPACE 1