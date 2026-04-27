/*
 * parser.c — Lê expressão (estilo .drg) e gera assembly Neander válido.
 * Suporta: atribuição opcional (variavel = expressão), +, *, parênteses.
 * O valor da expressão vai para o acumulador e depois para a variável (ou RESULT).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char *entrada;
static int posicao;
static int erro;

static void pular_espacos(void) {
    while (isspace((unsigned char)entrada[posicao]))
        posicao++;
}

static long ler_numero(void) {
    pular_espacos();
    if (!isdigit((unsigned char)entrada[posicao])) {
        erro = 1;
        return 0;
    }
    long n = 0;
    while (isdigit((unsigned char)entrada[posicao]))
        n = n * 10 + (entrada[posicao++] - '0');
    return n;
}

static long fator(void);
static long termo(void);
static long expressao(void);

static long fator(void) {
    pular_espacos();
    if (entrada[posicao] == '(') {
        posicao++;
        long r = expressao();
        pular_espacos();
        if (entrada[posicao] == ')')
            posicao++;
        else
            erro = 1;
        return r;
    }
    return ler_numero();
}

static long termo(void) {
    long r = fator();
    while (!erro) {
        pular_espacos();
        if (entrada[posicao] != '*')
            break;
        posicao++;
        r *= fator();
    }
    return r;
}

static long expressao(void) {
    long r = termo();
    while (!erro) {
        pular_espacos();
        if (entrada[posicao] != '+')
            break;
        posicao++;
        r += termo();
    }
    return r;
}

/* Lê nome (letra ou _ seguido de letras, números ou _). Retorna 1 se leu algo. */
static int ler_nome_variavel(char *nome, int tamanho_max) {
    pular_espacos();
    int i = 0;
    if (!isalpha((unsigned char)entrada[posicao]) && entrada[posicao] != '_')
        return 0;
    while (i < tamanho_max - 1) {
        char c = entrada[posicao];
        if (!isalnum((unsigned char)c) && c != '_')
            break;
        nome[i++] = c;
        posicao++;
    }
    nome[i] = '\0';
    return i > 0;
}

static int nome_reservado(const char *n) {
    static const char *lista[] = {"ORG", "DATA", "SPACE", "LDA", "STA", "ADD", "JMP", "JZ", "JN",
                                  "HLT", "NOP", "NOT", "AND", "OR"};
    for (size_t k = 0; k < sizeof lista / sizeof lista[0]; k++) {
        const char *a = n, *b = lista[k];
        while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) {
            a++;
            b++;
        }
        if (*a == *b)
            return 1;
    }
    return 0;
}

/*
 * Se a linha for "nome = expressao", guarda nome e avalia só a parte direita.
 * Senão, nome_saida = RESULT e a string inteira é a expressão (retrocede antes do nome lido).
 */
static long interpretar(char *texto, char *nome_saida, int max_nome) {
    entrada = texto;
    posicao = 0;
    erro = 0;

    pular_espacos();
    int inicio_expressao = posicao;

    char temp[32];
    if (ler_nome_variavel(temp, sizeof temp)) {
        pular_espacos();
        if (entrada[posicao] == '=' && !nome_reservado(temp)) {
            posicao++;
            strncpy(nome_saida, temp, (size_t)max_nome - 1);
            nome_saida[max_nome - 1] = '\0';
            long valor = expressao();
            pular_espacos();
            if (entrada[posicao] != '\0')
                erro = 1;
            return valor;
        }
        posicao = inicio_expressao;
    }

    strncpy(nome_saida, "RESULT", (size_t)max_nome);
    nome_saida[max_nome - 1] = '\0';
    long valor = expressao();
    pular_espacos();
    if (entrada[posicao] != '\0')
        erro = 1;
    return valor;
}

/* Assembly mínimo e válido: constante em VALOR, resultado em nome_variavel. */
static void escrever_arquivo_asm(FILE *arquivo, const char *nome_variavel, long valor) {
    long byte = valor & 0xFF;
    fprintf(arquivo, "; Gerado pelo parser a partir da expressao\n");
    fprintf(arquivo, "ORG 0\n");
    fprintf(arquivo, "    LDA VALOR\n");
    fprintf(arquivo, "    STA %s\n", nome_variavel);
    fprintf(arquivo, "    HLT\n");
    fprintf(arquivo, "VALOR DATA %ld\n", byte);
    fprintf(arquivo, "%s SPACE 1\n", nome_variavel);
}

static void mostrar_uso(const char *programa) {
    fprintf(stderr,
            "Uso:\n"
            "  %s \"expressao\"                 (só mostra o resultado)\n"
            "  %s -o saida.asm \"expressao\"    (gera .asm; sem \"=\" usa RESULT)\n"
            "  %s \"v = expressao\" saida.asm  (expressao e nome do arquivo)\n",
            programa, programa, programa);
}

long avaliar(char *texto) {
    char nome[32];
    long v = interpretar(texto, nome, sizeof nome);
    (void)nome;
    return erro ? 0 : v;
}

int main(int argc, char **argv) {
    char nome[32];

    if (argc == 2) {
        long v = avaliar(argv[1]);
        if (erro) {
            fprintf(stderr, "Erro de sintaxe na expressao.\n");
            return 1;
        }
        printf("Resultado = %ld\n", v);
        return 0;
    }

    if (argc == 4 && strcmp(argv[1], "-o") == 0) {
        long v = interpretar(argv[3], nome, sizeof nome);
        if (erro) {
            fprintf(stderr, "Erro de sintaxe na expressao.\n");
            return 1;
        }
        FILE *saida = fopen(argv[2], "w");
        if (!saida) {
            perror(argv[2]);
            return 1;
        }
        escrever_arquivo_asm(saida, nome, v);
        fclose(saida);
        printf("Assembly gerado em: %s\n", argv[2]);
        return 0;
    }

    if (argc == 3) {
        long v = interpretar(argv[1], nome, sizeof nome);
        if (erro) {
            fprintf(stderr, "Erro de sintaxe na expressao.\n");
            return 1;
        }
        FILE *saida = fopen(argv[2], "w");
        if (!saida) {
            perror(argv[2]);
            return 1;
        }
        escrever_arquivo_asm(saida, nome, v);
        fclose(saida);
        printf("Arquivo gerado: %s\n", argv[2]);
        return 0;
    }

    mostrar_uso(argv[0]);
    return 1;
}
