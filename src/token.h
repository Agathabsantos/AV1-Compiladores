#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

typedef enum {
    TOKEN_ID,
    TOKEN_NUM,
    TOKEN_MAIS,
    TOKEN_VEZES,
    TOKEN_IGUAL,
    TOKEN_ABRE,
    TOKEN_FECHA,
    TOKEN_FIM,
    TOKEN_INVALIDO
} TipoToken;

typedef struct {
    TipoToken tipo;
    char texto[64];
} Token;

typedef struct {
    char nome[32];
    long valor;
} Variavel;

extern Token lista_tokens[256];
extern int total_tokens;

void analisar_lexico(char *linha);

int analisar_instrucao(char *linha,
                       char *nome_destino,
                       size_t tamanho_nome,
                       long *valor,
                       char *mensagem_erro,
                       size_t tamanho_erro);

void limpar_variaveis(void);
int definir_variavel(const char *nome, long valor);
int obter_variavel(const char *nome, long *valor);
int obter_total_variaveis(void);
Variavel obter_variavel_por_indice(int indice);

#endif
