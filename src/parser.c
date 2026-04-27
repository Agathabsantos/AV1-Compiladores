#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "token.h"

#define MAX_VARIAVEIS 128

static int posicao;
static int erro;
static char mensagem[128];
static Variavel tabela_variaveis[MAX_VARIAVEIS];
static int total_variaveis = 0;

static Token atual(void) {
    return lista_tokens[posicao];
}

static Token proximo(void) {
    if (posicao + 1 < total_tokens)
        return lista_tokens[posicao + 1];
    return lista_tokens[posicao];
}

static void avancar(void) {
    if (posicao < total_tokens)
        posicao++;
}

static int mesmo_texto_sem_caixa(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        a++;
        b++;
    }
    return *a == *b;
}

static int nome_reservado(const char *nome) {
    static const char *reservadas[] = {
        "ORG", "DATA", "SPACE", "LDA", "STA", "ADD", "JMP", "JZ", "JN",
        "HLT", "NOP", "NOT", "AND", "OR"
    };
    size_t total = sizeof(reservadas) / sizeof(reservadas[0]);
    for (size_t i = 0; i < total; i++) {
        if (mesmo_texto_sem_caixa(nome, reservadas[i]))
            return 1;
    }
    return 0;
}

void limpar_variaveis(void) {
    total_variaveis = 0;
}

int definir_variavel(const char *nome, long valor) {
    for (int i = 0; i < total_variaveis; i++) {
        if (mesmo_texto_sem_caixa(tabela_variaveis[i].nome, nome)) {
            tabela_variaveis[i].valor = valor;
            return 1;
        }
    }
    if (total_variaveis >= MAX_VARIAVEIS)
        return 0;
    strncpy(tabela_variaveis[total_variaveis].nome, nome, sizeof(tabela_variaveis[total_variaveis].nome) - 1);
    tabela_variaveis[total_variaveis].nome[sizeof(tabela_variaveis[total_variaveis].nome) - 1] = '\0';
    tabela_variaveis[total_variaveis].valor = valor;
    total_variaveis++;
    return 1;
}

int obter_variavel(const char *nome, long *valor) {
    for (int i = 0; i < total_variaveis; i++) {
        if (mesmo_texto_sem_caixa(tabela_variaveis[i].nome, nome)) {
            *valor = tabela_variaveis[i].valor;
            return 1;
        }
    }
    return 0;
}

int obter_total_variaveis(void) {
    return total_variaveis;
}

Variavel obter_variavel_por_indice(int indice) {
    Variavel vazia;
    vazia.nome[0] = '\0';
    vazia.valor = 0;
    if (indice < 0 || indice >= total_variaveis)
        return vazia;
    return tabela_variaveis[indice];
}

static void marcar_erro(const char *texto) {
    if (erro)
        return;
    erro = 1;
    strncpy(mensagem, texto, sizeof(mensagem) - 1);
    mensagem[sizeof(mensagem) - 1] = '\0';
}

static long fator(void);
static long termo(void);
static long expressao(void);

// Gramática (precedência):
// fator -> NUM | ID | '(' expressao ')'
// termo -> fator ('*' fator)*
// expressao -> termo ('+' termo)*
static long fator(void) {
    Token t = atual();

    if (t.tipo == TOKEN_NUM) {
        avancar();
        return strtol(t.texto, NULL, 10);
    }

    if (t.tipo == TOKEN_ID) {
        long valor = 0;
        if (!obter_variavel(t.texto, &valor))
            marcar_erro("Variavel usada antes de receber valor");
        avancar();
        return valor;
    }

    if (t.tipo == TOKEN_ABRE) {
        avancar();
        long valor = expressao();
        if (atual().tipo != TOKEN_FECHA)
            marcar_erro("Era esperado ')' na expressao");
        else
            avancar();
        return valor;
    }

    marcar_erro("Era esperado numero, variavel ou '(' na expressao");
    return 0;
}

static long termo(void) {
    long valor = fator();
    while (!erro && atual().tipo == TOKEN_VEZES) {
        avancar();
        valor *= fator();
    }
    return valor;
}

static long expressao(void) {
    long valor = termo();
    while (!erro && atual().tipo == TOKEN_MAIS) {
        avancar();
        valor += termo();
    }
    return valor;
}

int analisar_instrucao(char *linha,
                       char *nome_destino,
                       size_t tamanho_nome,
                       long *valor,
                       char *mensagem_erro,
                       size_t tamanho_erro) {
    analisar_lexico(linha);

    posicao = 0;
    erro = 0;
    mensagem[0] = '\0';

    if (atual().tipo == TOKEN_INVALIDO)
        marcar_erro("Caractere invalido na entrada");

    // Aceita "nome = expressao" ou só "expressao" (resultado em RESULT).
    if (!erro && atual().tipo == TOKEN_ID && proximo().tipo == TOKEN_IGUAL) {
        if (nome_reservado(atual().texto)) {
            marcar_erro("Nome de variavel reservado");
        } else {
            strncpy(nome_destino, atual().texto, tamanho_nome - 1);
            nome_destino[tamanho_nome - 1] = '\0';
            avancar();
            avancar();
        }
    } else {
        strncpy(nome_destino, "RESULT", tamanho_nome - 1);
        nome_destino[tamanho_nome - 1] = '\0';
    }

    if (!erro)
        *valor = expressao();

    if (!erro && atual().tipo != TOKEN_FIM)
        marcar_erro("Sobrou texto apos o fim da expressao");

    if (erro) {
        strncpy(mensagem_erro, mensagem, tamanho_erro - 1);
        mensagem_erro[tamanho_erro - 1] = '\0';
        return 0;
    }

    mensagem_erro[0] = '\0';
    return 1;
}
