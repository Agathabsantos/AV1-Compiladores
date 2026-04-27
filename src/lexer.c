#include <string.h>
#include <ctype.h>

#include "token.h"

Token lista_tokens[256];
int total_tokens = 0;

static char *entrada;

static void adicionar_token(TipoToken tipo, const char *texto) {
    if (total_tokens >= 256)
        return;
    lista_tokens[total_tokens].tipo = tipo;
    strncpy(lista_tokens[total_tokens].texto, texto, sizeof(lista_tokens[total_tokens].texto) - 1);
    lista_tokens[total_tokens].texto[sizeof(lista_tokens[total_tokens].texto) - 1] = '\0';
    total_tokens++;
}

static void pular_espacos(void) {
    while (*entrada == ' ' || *entrada == '\t' || *entrada == '\n' || *entrada == '\r')
        entrada++;
}

void analisar_lexico(char *linha) {
    entrada = linha;
    total_tokens = 0;

    while (*entrada) {
        pular_espacos();

        if (*entrada == '\0')
            break;

        if (isalpha((unsigned char)*entrada) || *entrada == '_') {
            char buffer[64];
            int i = 0;
            while ((isalnum((unsigned char)*entrada) || *entrada == '_') && i < 63)
                buffer[i++] = *entrada++;
            buffer[i] = '\0';
            adicionar_token(TOKEN_ID, buffer);
            continue;
        }

        if (isdigit((unsigned char)*entrada)) {
            char buffer[64];
            int i = 0;
            while (isdigit((unsigned char)*entrada) && i < 63)
                buffer[i++] = *entrada++;
            buffer[i] = '\0';
            adicionar_token(TOKEN_NUM, buffer);
            continue;
        }

        if (*entrada == '+') {
            adicionar_token(TOKEN_MAIS, "+");
            entrada++;
            continue;
        }

        if (*entrada == '*') {
            adicionar_token(TOKEN_VEZES, "*");
            entrada++;
            continue;
        }

        if (*entrada == '=') {
            adicionar_token(TOKEN_IGUAL, "=");
            entrada++;
            continue;
        }

        if (*entrada == '(') {
            adicionar_token(TOKEN_ABRE, "(");
            entrada++;
            continue;
        }

        if (*entrada == ')') {
            adicionar_token(TOKEN_FECHA, ")");
            entrada++;
            continue;
        }

        {
            char texto[2] = {*entrada, '\0'};
            adicionar_token(TOKEN_INVALIDO, texto);
            entrada++;
        }
    }

    adicionar_token(TOKEN_FIM, "EOF");
}
