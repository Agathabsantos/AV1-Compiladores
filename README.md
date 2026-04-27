# Avaliação 1 - Compiladores

Projeto da disciplina de Compiladores com implementação de:

- `assembler.c` (montador para a máquina Neander);
- `executor.c` (simulador/executor da Neander);
- `parser.c` (parser de expressões matemáticas básicas).

## Objetivo

O objetivo do projeto é montar programas em assembly da Neander, gerar memória de saída e executar os programas em um simulador, incluindo testes de operações e saltos condicionais.

## Estrutura do projeto

- `assembler.c`
  - faz montagem em duas passagens;
  - monta tabela de símbolos;
  - valida erros básicos de sintaxe;
  - gera `.mem` no formato NDR.
- `executor.c`
  - carrega arquivo NDR;
  - executa ciclo fetch/decode/execute;
  - mostra estado final de registradores e memória.
- `parser.c`
  - parser legado em arquivo único (mantido como referência).
- `src/`
  - `token.h`, `lexer.c`, `parser.c`, `drg_compiler.c` (versão modular usada para gerar o binário `parser`);
  - suporta expressões com `+`, `*`, parênteses, atribuição opcional (`v = expr`) e leitura de arquivo `.drg` com múltiplas linhas.
- `testes/`
  - `teste_calculo_simples.drg`: exemplo do item 4 (cálculo simples);
  - `teste_salto_condicional.drg`: exemplo do item 4 (condicional com `if/while`, gerando `JMP`, `JN`, `JZ`);
  - arquivos `.asm` e `.mem` gerados durante os testes.
- `Relatorio.md`
  - breve relatório com a estrutura do assembler e o funcionamento do executor.
- `Makefile`
  - automação de compilação e testes.

## Pré-requisitos

- GCC
- Make
- Linux/WSL (recomendado para rodar os comandos abaixo)

No Ubuntu/WSL, se necessário:

```bash
sudo apt update
sudo apt install -y build-essential make
```

## Compilação

Com Makefile:

```bash
make
```

Sem Makefile:

```bash
gcc -o assembler assembler.c
gcc -o executor executor.c
gcc -o parser src/drg_compiler.c src/lexer.c src/parser.c
```

## Execução dos testes

Todos os testes:

```bash
make test
```

Testes do item 4 (sugestão do enunciado):

```bash
make item4
```

Ou separados:

```bash
make item4-calculo
make item4-condicional
```

Pipeline parser -> assembler -> executor:

```bash
make pipeline
```

Compilar um arquivo `.drg`:

```bash
./parser testes/teste_calculo_simples.drg testes/teste_calculo_simples.asm
./assembler testes/teste_calculo_simples.asm testes/teste_calculo_simples.mem
./executor testes/teste_calculo_simples.mem decimal
```

## Fluxo manual (assembler + executor)

Montar:

```bash
./assembler testes/teste_calculo_simples.asm testes/teste_calculo_simples.mem
```

Executar:

```bash
./executor testes/teste_calculo_simples.mem decimal
```

Também é possível usar `hex` no lugar de `decimal`:

```bash
./executor testes/teste_calculo_simples.mem hex
```

## Compatibilidade com GUI Neander

Os arquivos `.mem` gerados estão no formato NDR e podem ser carregados diretamente no GUI da Neander.

## Limpeza de artefatos

Para remover binários e arquivos de saída:

```bash
make clean
```

## Observações

- O `parser` compilado pelo `Makefile` usa os módulos de `src/`.
- O fluxo principal validado nos testes foi `.drg -> parser -> .asm -> assembler -> .mem -> executor` e também carregamento no GUI Neander.
