# Relatório - Assembler e Executor Neander

## 1. Visão geral

Este trabalho implementa dois módulos principais para a Máquina Neander:

- `assembler.c`: traduz código assembly para arquivo de memória no formato NDR.
- `executor.c`: carrega o arquivo de memória e simula a execução das instruções da máquina.

O objetivo foi construir um fluxo completo de montagem e execução de programas, com validação de sintaxe no assembler e simulação do ciclo de máquina no executor.

---

## 1.1 Parser

Além do arquivo legado `parser.c`, foi organizada uma versão modular em `src/` com:

- `lexer.c`: análise léxica e geração de tokens;
- `parser.c`: análise sintática/semântica da expressão;
- `token.h`: tipos de token e assinaturas compartilhadas;
- `drg_compiler.c`: programa principal para avaliar expressão e gerar `.asm`.

Essa organização deixa o fluxo mais próximo do modelo visto em aula (fases separadas) sem aumentar a complexidade da linguagem suportada.

Na versão atual, o módulo principal aceita também arquivo `.drg` com múltiplas linhas (por exemplo `a = 10`, `b = 20`, `c = a + b`), mantendo valores de variáveis entre linhas durante a análise e gerando um `.asm` único para o `assembler`.

Para os exemplos do item 4 do enunciado, os arquivos ficam centralizados em `testes/`:
- `teste_calculo_simples.drg` (cálculo simples);
- `teste_salto_condicional.drg` (condicional/laço em `.drg`, gerando `.asm` com `JMP`, `JN` e `JZ`).

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

O assembler e o executor foram implementados de forma funcional e integrada:

- o assembler gera arquivo de memória no formato NDR;
- o executor carrega esse arquivo e executa corretamente os programas;
- os testes de soma e saltos condicionais validaram o comportamento esperado.

Com isso, os requisitos de documentação sobre estrutura do assembler e funcionamento do executor foram atendidos.
