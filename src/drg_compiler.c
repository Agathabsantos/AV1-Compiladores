#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "token.h"

typedef struct {
    char destino[32];
    long valor;
} InstrucaoGerada;

typedef struct {
    long valor;
    char rotulo[16];
} MapaConstante;

static MapaConstante constantes_dinamicas[256];
static int total_constantes_dinamicas = 0;
static int sequencia_rotulos = 0;

static int linha_vazia_ou_comentario(const char *linha) {
    while (*linha == ' ' || *linha == '\t' || *linha == '\n' || *linha == '\r')
        linha++;
    return *linha == '\0' || *linha == ';' || *linha == '#';
}

static void remover_quebra_linha(char *linha) {
    linha[strcspn(linha, "\r\n")] = '\0';
}

static char *pular_espacos(char *texto) {
    while (*texto == ' ' || *texto == '\t')
        texto++;
    return texto;
}

static const char *obter_rotulo_constante(long valor) {
    // Reaproveita rótulo já existente para não duplicar DATA no final do arquivo.
    for (int i = 0; i < total_constantes_dinamicas; i++) {
        if (constantes_dinamicas[i].valor == valor)
            return constantes_dinamicas[i].rotulo;
    }
    if (total_constantes_dinamicas >= 256)
        return NULL;
    constantes_dinamicas[total_constantes_dinamicas].valor = valor;
    snprintf(constantes_dinamicas[total_constantes_dinamicas].rotulo,
             sizeof(constantes_dinamicas[total_constantes_dinamicas].rotulo),
             "CONST_%d", total_constantes_dinamicas);
    total_constantes_dinamicas++;
    return constantes_dinamicas[total_constantes_dinamicas - 1].rotulo;
}

static int token_numero(const char *texto, long *valor) {
    char *fim = NULL;
    *valor = strtol(texto, &fim, 10);
    return fim != texto && *fim == '\0';
}

static int token_identificador(const char *texto) {
    if (!texto || *texto == '\0')
        return 0;
    if (!((*texto >= 'A' && *texto <= 'Z') || (*texto >= 'a' && *texto <= 'z') || *texto == '_'))
        return 0;
    for (int i = 1; texto[i] != '\0'; i++) {
        char c = texto[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'))
            return 0;
    }
    return 1;
}

static int emitir_atribuicao_dinamica(FILE *saida, char *linha, char *erro) {
    char destino[32], op1[32], op2[32] = {0};
    char operador = 0;
    long numero = 0;
    int campos = sscanf(linha, " %31[^= ] = %31s %c %31s", destino, op1, &operador, op2);
    if (campos < 2) {
        strcpy(erro, "atribuicao invalida");
        return 0;
    }

    // Registra a variável para garantir que ela seja declarada com SPACE no fim.
    definir_variavel(destino, 0);

    if (token_numero(op1, &numero)) {
        const char *c = obter_rotulo_constante(numero);
        if (!c) { strcpy(erro, "muitas constantes"); return 0; }
        fprintf(saida, "    LDA %s\n", c);
    } else if (token_identificador(op1)) {
        definir_variavel(op1, 0);
        fprintf(saida, "    LDA %s\n", op1);
    } else {
        strcpy(erro, "operando invalido");
        return 0;
    }

    if (campos == 4) {
        if (token_numero(op2, &numero)) {
            if (operador == '-')
                numero = -numero;
            else if (operador != '+') {
                strcpy(erro, "operador invalido");
                return 0;
            }
            const char *c = obter_rotulo_constante(numero);
            if (!c) { strcpy(erro, "muitas constantes"); return 0; }
            fprintf(saida, "    ADD %s\n", c);
        } else if (operador == '+' && token_identificador(op2)) {
            definir_variavel(op2, 0);
            fprintf(saida, "    ADD %s\n", op2);
        } else {
            strcpy(erro, "use '+' com variavel ou '-' com numero");
            return 0;
        }
    }

    fprintf(saida, "    STA %s\n", destino);
    return 1;
}

static void escrever_programa(FILE *saida, InstrucaoGerada *instrucoes, int total_instrucoes) {
    fprintf(saida, "; Gerado pelo compilador simples (.drg -> .asm)\n");
    fprintf(saida, "ORG 0\n");

    for (int i = 0; i < total_instrucoes; i++) {
        fprintf(saida, "    LDA CONST_%d\n", i);
        fprintf(saida, "    STA %s\n", instrucoes[i].destino);
    }

    fprintf(saida, "    HLT\n");

    for (int i = 0; i < total_instrucoes; i++)
        fprintf(saida, "CONST_%d DATA %ld\n", i, instrucoes[i].valor & 0xFF);

    for (int i = 0; i < obter_total_variaveis(); i++) {
        Variavel v = obter_variavel_por_indice(i);
        fprintf(saida, "%s SPACE 1\n", v.nome);
    }
}

static int gerar_de_expressao_unica(const char *saida_asm, char *expressao) {
    char nome_destino[32];
    char erro[128];
    long valor = 0;
    InstrucaoGerada instrucao;

    limpar_variaveis();

    if (!analisar_instrucao(expressao, nome_destino, sizeof(nome_destino), &valor, erro, sizeof(erro))) {
        fprintf(stderr, "Erro: %s\n", erro);
        return 0;
    }
    if (!definir_variavel(nome_destino, valor)) {
        fprintf(stderr, "Erro: limite de variaveis excedido.\n");
        return 0;
    }

    strncpy(instrucao.destino, nome_destino, sizeof(instrucao.destino) - 1);
    instrucao.destino[sizeof(instrucao.destino) - 1] = '\0';
    instrucao.valor = valor;

    FILE *saida = fopen(saida_asm, "w");
    if (!saida) {
        perror(saida_asm);
        return 0;
    }

    escrever_programa(saida, &instrucao, 1);
    fclose(saida);
    return 1;
}

static int gerar_de_arquivo_drg(const char *arquivo_drg, const char *saida_asm) {
    FILE *entrada = fopen(arquivo_drg, "r");
    if (!entrada) {
        perror(arquivo_drg);
        return 0;
    }

    FILE *saida = fopen(saida_asm, "w");
    if (!saida) {
        perror(saida_asm);
        fclose(entrada);
        return 0;
    }

    char linha[256];
    int numero_linha = 0;
    char erro[128];

    limpar_variaveis();
    total_constantes_dinamicas = 0;
    sequencia_rotulos = 0;
    fprintf(saida, "; Gerado pelo compilador simples (.drg -> .asm)\n");
    fprintf(saida, "ORG 0\n");

    while (fgets(linha, sizeof(linha), entrada)) {
        numero_linha++;
        remover_quebra_linha(linha);
        char *atual = pular_espacos(linha);
        if (linha_vazia_ou_comentario(atual))
            continue;

        if (strncmp(atual, "if ", 3) == 0 || strcmp(atual, "if") == 0) {
            char cond[32], rotulo_fim[32];
            if (sscanf(atual, "if %31s", cond) != 1) {
                fprintf(stderr, "Erro na linha %d: if invalido\n", numero_linha);
                fclose(entrada); fclose(saida); return 0;
            }
            if (!fgets(linha, sizeof(linha), entrada)) {
                fprintf(stderr, "Erro na linha %d: if sem corpo\n", numero_linha);
                fclose(entrada); fclose(saida); return 0;
            }
            numero_linha++;
            remover_quebra_linha(linha);
            snprintf(rotulo_fim, sizeof(rotulo_fim), "FIM_IF_%d", sequencia_rotulos++);
            definir_variavel(cond, 0);
            // Convenção usada: se cond for negativa ou zero, não executa o bloco.
            fprintf(saida, "    LDA %s\n    JN %s\n    JZ %s\n", cond, rotulo_fim, rotulo_fim);
            if (!emitir_atribuicao_dinamica(saida, pular_espacos(linha), erro)) {
                fprintf(stderr, "Erro na linha %d: %s\n", numero_linha, erro);
                fclose(entrada); fclose(saida); return 0;
            }
            fprintf(saida, "%s:\n", rotulo_fim);
            continue;
        }

        if (strncmp(atual, "while ", 6) == 0 || strcmp(atual, "while") == 0) {
            char cond[32], rotulo_ini[32], rotulo_fim[32];
            if (sscanf(atual, "while %31s", cond) != 1) {
                fprintf(stderr, "Erro na linha %d: while invalido\n", numero_linha);
                fclose(entrada); fclose(saida); return 0;
            }
            if (!fgets(linha, sizeof(linha), entrada)) {
                fprintf(stderr, "Erro na linha %d: while sem corpo\n", numero_linha);
                fclose(entrada); fclose(saida); return 0;
            }
            numero_linha++;
            remover_quebra_linha(linha);
            snprintf(rotulo_ini, sizeof(rotulo_ini), "INI_WHILE_%d", sequencia_rotulos);
            snprintf(rotulo_fim, sizeof(rotulo_fim), "FIM_WHILE_%d", sequencia_rotulos++);
            definir_variavel(cond, 0);
            // while cond: repete enquanto cond > 0.
            fprintf(saida, "%s:\n    LDA %s\n    JN %s\n    JZ %s\n", rotulo_ini, cond, rotulo_fim, rotulo_fim);
            if (!emitir_atribuicao_dinamica(saida, pular_espacos(linha), erro)) {
                fprintf(stderr, "Erro na linha %d: %s\n", numero_linha, erro);
                fclose(entrada); fclose(saida); return 0;
            }
            fprintf(saida, "    JMP %s\n%s:\n", rotulo_ini, rotulo_fim);
            continue;
        }

        if (!emitir_atribuicao_dinamica(saida, atual, erro)) {
            fprintf(stderr, "Erro na linha %d: %s\n", numero_linha, erro);
            fclose(entrada);
            fclose(saida);
            return 0;
        }
    }

    fprintf(saida, "    HLT\n");
    for (int i = 0; i < total_constantes_dinamicas; i++)
        fprintf(saida, "%s DATA %ld\n", constantes_dinamicas[i].rotulo, constantes_dinamicas[i].valor);
    for (int i = 0; i < obter_total_variaveis(); i++) {
        Variavel v = obter_variavel_por_indice(i);
        fprintf(saida, "%s SPACE 1\n", v.nome);
    }
    fclose(entrada);
    fclose(saida);
    return 1;
}

static void mostrar_uso(const char *programa) {
    fprintf(stderr,
            "Uso:\n"
            "  %s \"expressao\"\n"
            "  %s -o saida.asm \"expressao\"\n"
            "  %s \"variavel = expressao\" saida.asm\n"
            "  %s arquivo.drg saida.asm\n",
            programa, programa, programa, programa);
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        char nome_destino[32];
        char erro[128];
        long valor = 0;
        limpar_variaveis();
        if (!analisar_instrucao(argv[1], nome_destino, sizeof(nome_destino), &valor, erro, sizeof(erro))) {
            fprintf(stderr, "Erro: %s\n", erro);
            return 1;
        }
        printf("Resultado = %ld\n", valor);
        return 0;
    }

    if (argc == 4 && strcmp(argv[1], "-o") == 0) {
        if (!gerar_de_expressao_unica(argv[2], argv[3]))
            return 1;
        printf("Assembly gerado em: %s\n", argv[2]);
        return 0;
    }

    if (argc == 3) {
        // Modo automático: se o primeiro argumento existir como arquivo, trata como .drg.
        // Caso contrário, mantém compatibilidade com o modo de expressão única.
        FILE *teste = fopen(argv[1], "r");
        if (teste) {
            fclose(teste);
            if (!gerar_de_arquivo_drg(argv[1], argv[2]))
                return 1;
        } else {
            if (!gerar_de_expressao_unica(argv[2], argv[1]))
                return 1;
        }
        printf("Arquivo gerado: %s\n", argv[2]);
        return 0;
    }

    mostrar_uso(argv[0]);
    return 1;
}
