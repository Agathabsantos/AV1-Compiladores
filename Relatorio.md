# Relatório - Assembler e Executor Neander

## 1. Visão geral

Este trabalho implementa três módulos integrados para a Máquina Neander:

- `assembler.c`: traduz código assembly para arquivo de memória no formato NDR.
- `executor.c`: carrega o arquivo de memória e simula a execução das instruções da máquina.
- parser modular em `src/`: traduz `.drg` para `.asm` no fluxo principal.

O objetivo foi construir um fluxo completo de montagem e execução de programas, com validação de sintaxe no assembler e simulação do ciclo de máquina no executor.

---

## 1.1 Parser e Gramática EBNF

Foi usada uma pequena linguagem em arquivos **`.drg`** para descrever cálculos e estruturas de controle simples. A implementação fica modular: o **`lexer.c`** gera tokens, o **`parser.c`** faz a análise sintática de **expressões** por descida recursiva e o **`drg_compiler.c`** percorre o programa `.drg`, detecta `if`/`while` e emite assembly Neander (`LDA`, `ADD`, `STA`, `JMP`, `JN`, `JZ`, `HLT`), reservando constantes (`DATA`) e variáveis (`SPACE`).

A seguir, uma **versão resumida** da gramática (termos entre aspas são literais; espaços entre tokens são opcionais na entrada real):

```
<programa> ::= { <instrução> }

<instrução> ::= <atribuição>
             | "if " <variável> <nova_linha> <atribuição>
             | "while " <variável> <nova_linha> <atribuição>

<atribuição> ::= <variável> "=" <expressão>

<expressão> ::= <termo> { "+" <termo> }

<termo>       ::= <fator> { "*" <fator> }

<fator>       ::= <número> | <variável> | "(" <expressão> ")"

<variável>    ::= ( <letra> | "_" ) { <letra> | <dígito> | "_" }

<número>      ::= <dígito> { <dígito> }

<dígito>      ::= "0" | "1" | ... | "9"

<letra>       ::= "a" | "b" | ... | "z" | "A" | "B" | ... | "Z"
```

O símbolo **`<nova_linha>`** indica que, em `if` e `while`, o **corpo** (uma atribuição) aparece na **linha seguinte** no arquivo — como nos exemplos da pasta `testes/`. Linhas vazias ou só com comentário (`;` ou `#`) são ignoradas.

Observações rápidas para alinhar gramática e código: (i) o **`parser.c`** também aceita uma linha só com **expressão** (sem `=`), resultado associado ao nome `RESULT` na avaliação isolada; (ii) alguns identificadores são **reservados** (mnemônicos como `LDA`, `ORG`, …) e são rejeitados como nome de variável; (iii) **`notação opcional com menos`** na frente de literal (`<número>` com sinal) não existe como um único token no lexer — quando há subtração de um valor constante em uma atribuição no `.drg`, ela é tratada pelo compilador como combinação `ADD` com constante negativa.

Na versão atual, o módulo principal aceita também arquivo `.drg` com múltiplas linhas (por exemplo `a = 10`, `b = 20`, `c = a + b`), mantendo o controle das variáveis utilizadas ao longo das linhas e gerando um `.asm` único para o `assembler`.

Para os exemplos do item 4 do enunciado, os arquivos ficam centralizados em `testes/`:
- `teste_calculo_simples.drg` (cálculo simples);
- `teste_salto_condicional.drg` (condicional/laço em `.drg`, gerando `.asm` com `JMP`, `JN` e `JZ`).

Também foram incluídos exemplos em assembly manual para execução direta no `assembler`, sem necessidade de passar pelo parser:
- `asm_calculo_simples.asm`;
- `asm_salto_condicional.asm`.

A linguagem suportada é propositalmente simplificada, não contemplando uma árvore sintática abstrata (AST) completa, sendo suficiente para demonstrar o fluxo de compilação e geração de código.

---

## 2. Estrutura do Assembler

O assembler foi implementado em **duas passagens**, com uso de **tabela de símbolos** para resolução de rótulos.

### 2.1 Primeira passagem

Na primeira passagem, o código fonte assembly é percorrido linha a linha para:

- remover comentários e ignorar linhas vazias;
- identificar rótulos e montar a tabela de símbolos (rótulo -> endereço);
- processar diretivas (`ORG`, `DATA`, `SPACE`) para cálculo correto do `PC`;
- validar erros de sintaxe, como:
  - instrução inválida;
  - instrução com operando faltando ou em excesso;
  - diretivas com parâmetros inválidos;
  - rótulos duplicados.

Ao final dessa etapa, os endereços dos símbolos já estão definidos.

### 2.2 Tabela de símbolos

A tabela de símbolos armazena:

- nome do rótulo;
- endereço correspondente na memória.

Ela é essencial para instruções de salto e acesso a dados por nome, por exemplo `JMP FIM` ou `LDA VALOR`.

### 2.3 Segunda passagem

Na segunda passagem, o arquivo é percorrido novamente para:

- converter mnemônicos em opcodes;
- resolver operandos (numéricos ou por símbolo);
- escrever os bytes da memória final (`MEMORIA[256]`).

Depois da montagem, o assembler gera o arquivo de saída no padrão **NDR**:

- cabeçalho de 4 bytes (`03 4E 44 52`);
- 256 palavras de 16 bits em little-endian (byte útil + byte zero).

Esse formato é compatível com o GUI do Neander.

---

## 3. Funcionamento do Executor

O executor simula a Máquina Neander com:

- memória de 256 posições (`MEMORIA`);
- registradores `AC`, `PC`, `IR`, `MAR`, `MDR`;
- flags `N` (negativo) e `Z` (zero).

### 3.1 Carregamento

O arquivo de entrada é aberto em modo binário e validado pelo cabeçalho NDR.  
Em seguida, as 256 posições de memória são carregadas para a simulação.

### 3.2 Ciclo de máquina

A execução segue o ciclo clássico:

1. **Fetch**: busca da instrução apontada por `PC` e armazenamento em `IR`.
2. **Decode**: identificação do opcode e tipo de instrução.
3. **Execute**: execução da operação (aritmética, lógica, salto, armazenamento etc.).

As instruções implementadas incluem:

- `NOP`, `STA`, `LDA`, `ADD`, `OR`, `AND`, `NOT`, `JMP`, `JN`, `JZ`, `HLT`.

O executor suporta:

- execução contínua;
- execução passo a passo (`step`), mostrando estado de registradores a cada instrução.
- visualização de saída em formato **decimal** ou **hexadecimal** (definido por parâmetro na execução).

### 3.3 Manipulação de flags

Após operações que alteram o acumulador (`AC`), as flags são atualizadas:

- `Z = 1` quando `AC == 0`;
- `N = 1` quando `AC < 0`.

Essas flags controlam os saltos condicionais:

- `JZ`: salto se zero;
- `JN`: salto se negativo.

---

## 4. Conclusão

Os módulos foram implementados de forma funcional e integrada:

- o assembler gera arquivo de memória no formato NDR;
- o executor carrega esse arquivo e executa corretamente os programas;
- o parser modular em `src/` gera `.asm` a partir de `.drg`;
- os testes de soma e saltos condicionais (via parser e via assembly manual) validaram o comportamento esperado.

Com isso, os requisitos de documentação sobre estrutura do assembler e funcionamento do executor foram atendidos.
