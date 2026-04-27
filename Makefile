CC = gcc
CFLAGS = -Wall -Wextra -std=c11

ASM = assembler
EXEC = executor
PARSER = parser
PARSER_FONTES = src/drg_compiler.c src/lexer.c src/parser.c

.PHONY: all clean test pipeline item4 item4-calculo item4-condicional

all: $(ASM) $(EXEC) $(PARSER)

$(ASM): assembler.c
	$(CC) $(CFLAGS) -o $(ASM) assembler.c

$(EXEC): executor.c
	$(CC) $(CFLAGS) -o $(EXEC) executor.c

$(PARSER): $(PARSER_FONTES)
	$(CC) $(CFLAGS) -o $(PARSER) $(PARSER_FONTES)

test: item4

pipeline: all
	./$(PARSER) -o testes/gerado_pipeline.asm "10+5*2"
	./$(ASM) testes/gerado_pipeline.asm testes/gerado_pipeline.mem
	./$(EXEC) testes/gerado_pipeline.mem decimal

item4: item4-calculo item4-condicional

item4-calculo: all
	./$(PARSER) testes/teste_calculo_simples.drg testes/teste_calculo_simples.asm
	./$(ASM) testes/teste_calculo_simples.asm testes/teste_calculo_simples.mem
	./$(EXEC) testes/teste_calculo_simples.mem decimal

item4-condicional: all
	./$(PARSER) testes/teste_salto_condicional.drg testes/teste_salto_condicional.asm
	./$(ASM) testes/teste_salto_condicional.asm testes/teste_salto_condicional.mem
	./$(EXEC) testes/teste_salto_condicional.mem decimal

clean:
	rm -f $(ASM) $(EXEC) $(PARSER) testes/*.mem testes/gerado_pipeline.asm *.mem
