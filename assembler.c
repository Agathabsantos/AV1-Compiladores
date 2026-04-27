#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#define MAX_LABELS 100
int erro = 0;

typedef struct {
    char nome[20];
    uint8_t endereco;
} Simbolo;

Simbolo tabela[MAX_LABELS];
int total_simbolos = 0;

uint8_t MEMORIA[256];
uint8_t PC = 0;

// Remove qualquer texto após ';' para ignorar comentários em assembly.
void remover_comentarios(char *linha) {
    char *comentario = strchr(linha, ';');
    if (comentario) *comentario = '\0';
}

// OBS: Opcodes definidos conforme implementação do executor
// (tabela do material possuía inconsistências)
uint8_t obter_opcode(char *instrucao) {
    if (strcmp(instrucao, "LDA") == 0) return 0x20;
    if (strcmp(instrucao, "ADD") == 0) return 0x30;
    if (strcmp(instrucao, "STA") == 0) return 0x10;
    if (strcmp(instrucao, "JMP") == 0) return 0x80;
    if (strcmp(instrucao, "JZ")  == 0) return 0xA0;
    if (strcmp(instrucao, "JN")  == 0) return 0x90;
    if (strcmp(instrucao, "AND") == 0) return 0x50;
    if (strcmp(instrucao, "OR")  == 0) return 0x40;
    if (strcmp(instrucao, "NOT") == 0) return 0x60;
    if (strcmp(instrucao, "NOP") == 0) return 0x00;
    if (strcmp(instrucao, "HLT") == 0) return 0xF0;
    return 0xFF;
}

int usa_operando(uint8_t opcode) {
    return !(opcode == 0x00 || opcode == 0x60 || opcode == 0xF0);
}

int token_numerico(const char *s) {
    if (s == NULL || *s == '\0') return 0;
    int i = 0;
    if (s[i] == '-') i++;
    if (s[i] == '\0') return 0;
    for (; s[i] != '\0'; i++) {
        if (!isdigit((unsigned char)s[i])) return 0;
    }
    return 1;
}

int buscar_simbolo(char *nome) {
    for (int i = 0; i < total_simbolos; i++) {
        if (strcmp(tabela[i].nome, nome) == 0)
            return tabela[i].endereco;
    }
    return -1;
}

void adicionar_simbolo(char *nome, uint8_t endereco) {
    //verifica se limite da tabela de simbolos foi excedida
     if (total_simbolos >= MAX_LABELS) {
        printf("Erro: tabela de símbolos cheia\n");
        erro = 1;
        return;
    }

    //verifica simbolo duplicado
    if (buscar_simbolo(nome) != -1) {
        printf("Erro: rótulo duplicado: %s\n", nome);
        erro = 1;
        return;
    }

    strcpy(tabela[total_simbolos].nome, nome);
    tabela[total_simbolos].endereco = endereco;
    total_simbolos++;
}

// ====================== PRIMEIRA PASSAGEM ======================
// Objetivo: validar sintaxe e montar tabela de símbolos (rótulo -> endereço),
// sem gerar bytes finais ainda.
void primeira_passagem(FILE *arquivo) {
    char linha[100];
    PC = 0;
    total_simbolos = 0;

    while (fgets(linha, sizeof(linha), arquivo)) {
        remover_comentarios(linha);
        linha[strcspn(linha, "\r\n")] = 0;
        if (strlen(linha) == 0) continue;

        char *token = strtok(linha, " \t");
        if (token == NULL) continue;

        char *label = NULL;
        
        // Detecta label com ':'
        if (token[strlen(token)-1] == ':') {
            token[strlen(token)-1] = '\0';
            label = token;
            token = strtok(NULL, " \t");
            if (token == NULL) {
                adicionar_simbolo(label, PC);
                continue;
            }
        }

        // ORG
        if (strcmp(token, "ORG") == 0) {
            char *val = strtok(NULL, " \t");
            if (!val || !token_numerico(val)) {
                printf("Erro: ORG requer endereço numérico válido\n");
                erro = 1;
                continue;
            }
            if (strtok(NULL, " \t") != NULL) {
                printf("Erro: ORG possui argumentos em excesso\n");
                erro = 1;
                continue;
            }
            PC = (uint8_t)atoi(val);
            continue;
        }

        // Verifica se próximo token é DATA ou SPACE (label sem ':')
        char *proximo = strtok(NULL, " \t");
        
        if (proximo != NULL && strcmp(proximo, "DATA") == 0) {
            // token é o LABEL, proximo é DATA
            adicionar_simbolo(token, PC);
            char *val = strtok(NULL, " \t");
            if (!val || !token_numerico(val)) {
                printf("Erro: DATA requer valor numérico válido\n");
                erro = 1;
                continue;
            }
            if (strtok(NULL, " \t") != NULL) {
                printf("Erro: DATA possui argumentos em excesso\n");
                erro = 1;
                continue;
            }
            PC++;
            continue;
        }
        
        if (proximo != NULL && strcmp(proximo, "SPACE") == 0) {
            // token é o LABEL, proximo é SPACE
            adicionar_simbolo(token, PC);
            char *tam = strtok(NULL, " \t");
            if (!tam || !token_numerico(tam)) {
                printf("Erro: SPACE requer tamanho numérico válido\n");
                erro = 1;
                continue;
            }
            if (strtok(NULL, " \t") != NULL) {
                printf("Erro: SPACE possui argumentos em excesso\n");
                erro = 1;
                continue;
            }
            int tamanho = atoi(tam);
            if (tamanho < 0) {
                printf("Erro: SPACE não aceita tamanho negativo\n");
                erro = 1;
                continue;
            }
            PC += tamanho;
            continue;
        }

        // DATA ou SPACE (após label com ':')
        if (strcmp(token, "DATA") == 0) {
            if (label) {
                adicionar_simbolo(label, PC);
            }
            if (!proximo || !token_numerico(proximo)) {
                printf("Erro: DATA requer valor numérico válido\n");
                erro = 1;
                continue;
            }
            if (strtok(NULL, " \t") != NULL) {
                printf("Erro: DATA possui argumentos em excesso\n");
                erro = 1;
                continue;
            }
            PC++;
            continue;
        }
        
        if (strcmp(token, "SPACE") == 0) {
            if (label) {
                adicionar_simbolo(label, PC);
            }
            char *tam = proximo;
            if (!tam || !token_numerico(tam)) {
                printf("Erro: SPACE requer tamanho numérico válido\n");
                erro = 1;
                continue;
            }
            if (strtok(NULL, " \t") != NULL) {
                printf("Erro: SPACE possui argumentos em excesso\n");
                erro = 1;
                continue;
            }
            int tamanho = atoi(tam);
            if (tamanho < 0) {
                printf("Erro: SPACE não aceita tamanho negativo\n");
                erro = 1;
                continue;
            }
            PC += tamanho;
            continue;
        }

        // Instrução normal
        uint8_t opcode = obter_opcode(token);
        if (opcode == 0xFF) {
            printf("Erro: instrução inválida: %s\n", token);
            erro = 1;
            continue;
        }

        if (label) {
            adicionar_simbolo(label, PC);
        }

        if (usa_operando(opcode)) {
            if (proximo == NULL) {
                printf("Erro: instrução requer operando: %s\n", token);
                erro = 1;
                continue;
            }
            if (strtok(NULL, " \t") != NULL) {
                printf("Erro: instrução possui argumentos em excesso: %s\n", token);
                erro = 1;
                continue;
            }
            PC += 2; // opcode + operando
        } else {
            if (proximo != NULL) {
                printf("Erro: instrução não aceita operando: %s\n", token);
                erro = 1;
                continue;
            }
            PC++; // opcode
        }
    }
    rewind(arquivo);
}

// ====================== SEGUNDA PASSAGEM ======================
// Objetivo: com a tabela pronta, converter instruções para bytes em MEMORIA[].
void segunda_passagem(FILE *arquivo) {
    char linha[100];
    PC = 0;

    while (fgets(linha, sizeof(linha), arquivo)) {
        remover_comentarios(linha);
        linha[strcspn(linha, "\r\n")] = 0;
        if (strlen(linha) == 0) continue;

        char *token = strtok(linha, " \t");
        if (token == NULL) continue;

        if (token[strlen(token)-1] == ':') {
            token = strtok(NULL, " \t");
            if (token == NULL) continue;
        }

        if (strcmp(token, "ORG") == 0) {
            char *val = strtok(NULL, " \t");
            if (val) PC = (uint8_t)atoi(val);
            continue;
        }

        char *proximo = strtok(NULL, " \t");
        
        // LABEL DATA valor (sem ':')
        if (proximo != NULL && strcmp(proximo, "DATA") == 0) {
            char *val = strtok(NULL, " \t");
            MEMORIA[PC++] = (uint8_t)(val ? atoi(val) : 0);
            continue;
        }
        
        // LABEL SPACE tamanho (sem ':')
        if (proximo != NULL && strcmp(proximo, "SPACE") == 0) {
            char *tam = strtok(NULL, " \t");
            int tamanho = tam ? atoi(tam) : 0;
            for (int i = 0; i < tamanho; i++) MEMORIA[PC++] = 0;
            continue;
        }

        uint8_t opcode = obter_opcode(token);
        if (!usa_operando(opcode) && proximo != NULL) {
            printf("Erro: instrução não aceita operando: %s\n", token);
            erro = 1;
            continue;
        }

        if (opcode == 0xFF && strcmp(token, "DATA") != 0 && strcmp(token, "SPACE") != 0) {
            printf("Erro: instrução inválida: %s\n", token);
            erro = 1;
            continue;
        }

        // DATA (após ':')
        if (strcmp(token, "DATA") == 0) {
            char *val = proximo;
            MEMORIA[PC++] = (uint8_t)(val ? atoi(val) : 0);
            continue;
        }

        // SPACE (após ':')
        if (strcmp(token, "SPACE") == 0) {
            char *tam = proximo;
            int tamanho = tam ? atoi(tam) : 0;
            for (int i = 0; i < tamanho; i++) MEMORIA[PC++] = 0;
            continue;
        }

        // Instrução normal
        if (opcode != 0xFF) {
            MEMORIA[PC++] = opcode;

            char *operando = proximo;
            if (operando != NULL && usa_operando(opcode)) {
                int endereco;
                // Operando pode ser hexadecimal, decimal ou rótulo.
                if (operando[0] == '0' && (operando[1] == 'x' || operando[1] == 'X')) {
                    endereco = (int)strtol(operando, NULL, 16);
                } else if (isdigit(operando[0]) || (operando[0] == '-' && isdigit(operando[1]))) {
                    endereco = atoi(operando);
                } else {
                    endereco = buscar_simbolo(operando);
                    if (endereco == -1) {
                        printf("Erro: símbolo não encontrado: %s\n", operando);
                        erro = 1;
                        continue;
                    }
                }
                if (endereco < 0 || endereco > 255) {
                    printf("Erro: endereço fora do intervalo: %d\n", endereco);
                    erro = 1;
                    continue;
                }
                MEMORIA[PC++] = (uint8_t)endereco;
            }
        }
    }
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: ./assembler <entrada.asm> <saida.mem>\n");
        return 1;
    }

    FILE *arq = fopen(argv[1], "r");
    if (!arq) { printf("Erro ao abrir arquivo de entrada\n"); return 1; }

    printf("=== PRIMEIRA PASSAGEM ===\n");
    primeira_passagem(arq);

    printf("Tabela de símbolos:\n");
    for (int i = 0; i < total_simbolos; i++) {
        printf("%s -> %d\n", tabela[i].nome, tabela[i].endereco);
    }

    printf("\n=== SEGUNDA PASSAGEM ===\n");
    memset(MEMORIA, 0, sizeof(MEMORIA));
    segunda_passagem(arq);

    fclose(arq);

    if (erro) {
        printf("\nErro(s) encontrados. Abortando...\n");
        remove(argv[2]);
        return 1;
    }

    FILE *out = fopen(argv[2], "wb");
    if (!out) {
        printf("Erro ao criar arquivo de saída\n");
        return 1;
    }

    // Formato NDR (compatível com Neander GUI):
    // header 03 4E 44 52 + 256 palavras de 16 bits little-endian.
    uint8_t header[4] = {0x03, 0x4E, 0x44, 0x52};
    fwrite(header, 1, 4, out);

    for (int i = 0; i < 256; i++) {
        uint16_t palavra = MEMORIA[i];
        fwrite(&palavra, sizeof(uint16_t), 1, out);
    }
    fclose(out);

    printf("Arquivo .mem (formato NDR) gerado com sucesso!\n");
    return 0;
}