#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

//Registradores
int8_t AC = 0; //acumulador
uint8_t PC = 0; //program counter
uint8_t IR_instrucao;     // Instruction Register
uint8_t MAR_endereco;     // Memory Address Register
uint8_t MDR_dado;         // Memory Data Register
int Z = 0;      //flag zero
int N = 0;      //flag negativo

//Memória: 256 posições de 8bits
uint8_t MEMORIA[256];

void atualizar_flags() {
    Z = (AC == 0);
    N = (AC < 0);
}

void carregar_arquivo(char *caminho){
    FILE *arquivo = fopen(caminho, "rb");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo!\n");
        exit(1);
    }

    uint8_t header[4];
    if (fread(header, 1, 4, arquivo) != 4) {
        printf("Arquivo inválido!\n");
        fclose(arquivo);
        exit(1);
    }

    if (header[0] != 0x03 || header[1] != 0x4E ||
        header[2] != 0x44 || header[3] != 0x52) {
        printf("Arquivo inválido!\n");
        fclose(arquivo);
        exit(1);
    }

    for (int i = 0; i < 256; i++) {
        uint8_t par[2];
        if (fread(par, 1, 2, arquivo) != 2) {
            printf("Erro durante leitura do arquivo!\n");
            fclose(arquivo);
            exit(1);
        }
        MEMORIA[i] = par[0];
    }

    if (ferror(arquivo)) {
        printf("Erro durante leitura do arquivo!\n");
        fclose(arquivo);
        exit(1);
    }

    printf("Arquivo carregado com sucesso!\n");
    fclose(arquivo);
}

void carregar_programa(uint8_t *programa, int tamanho){
    for(int i = 0; i < tamanho; i++){
        MEMORIA[i] = programa[i];
    }
}

void imprimir_mapa(char *titulo, char *formato){
    printf("\n=== %s ===\n", titulo);
    for(int i = 0; i < 256; i++){
        if(strcmp(formato, "hex") == 0){
            printf("MEMORIA[0x%02X] = 0x%02X\n", i, MEMORIA[i]);
        } else {
            printf("MEMORIA[%3d] = %3d\n", i, MEMORIA[i]);
        }
    }
}

// leitura da memória (usa MAR e MDR)
uint8_t ler_memoria(uint8_t endereco) {
    MAR_endereco = endereco;
    MDR_dado = MEMORIA[MAR_endereco];
    return MDR_dado;
}

// escrita na memória (usa MAR e MDR)
void escrever_memoria(uint8_t endereco, uint8_t valor) {
    MAR_endereco = endereco;
    MDR_dado = valor;
    MEMORIA[MAR_endereco] = MDR_dado;
}

int main(int argc, char *argv[]){
    if(argc != 3 && argc != 4){
        printf("Uso: ./neander <arquivo.mem> <decimal|hex>\n");
        return 1;
    }
    char *arquivo = argv[1];
    char *formato = argv[2];

    if(strcmp(formato, "hex") != 0 && strcmp(formato, "decimal") != 0){
        printf("Formato inválido. Use 'decimal' ou 'hex'.\n");
        return 1;
    }

    carregar_arquivo(arquivo);

    PC = 0;
    int executando = 1;
    int acessos_memoria = 0;
    int instrucoes_executadas = 0;
    int modo_step = 0;

    if(argc == 4 && strcmp(argv[3], "step") == 0){
            modo_step = 1;
    }

    imprimir_mapa("Mapa de Memória ANTES da execução (0-255)", formato);

    while(executando){
        //  FETCH 
        IR_instrucao = ler_memoria(PC);
        acessos_memoria++;
        PC++;

        uint8_t instrucao = IR_instrucao;
        instrucoes_executadas++;

        if(modo_step){
            printf("[PC=%d] IR=0x%02X AC=%d Z=%d N=%d\n", PC-1, IR_instrucao, AC, Z, N);
            printf("\nPressione ENTER...");
            getchar();
        }

        // EXECUTE
        if(instrucao == 0x20){  //LDA
            uint8_t endereco = ler_memoria(PC);
            acessos_memoria++;
            PC++;
            AC = (int8_t)ler_memoria(endereco);
            acessos_memoria++;
            atualizar_flags();
        } 

        else if(instrucao == 0x30){ //ADD
            uint8_t endereco = ler_memoria(PC);
            acessos_memoria++;
            PC++;
            AC = (int8_t)(AC + ler_memoria(endereco));
            acessos_memoria++;
            atualizar_flags();
        }

        else if(instrucao == 0x10){ //STA, pega o valor o do acumulador e guarda em um endereço de memória
            uint8_t endereco = ler_memoria(PC);
            acessos_memoria++;
            PC++;
            escrever_memoria(endereco, (uint8_t)AC);
            acessos_memoria++;
        }

        else if(instrucao == 0x80){ //JMP, pula para o HLT
            uint8_t endereco = ler_memoria(PC);
            acessos_memoria++;
            PC++;
            PC = endereco;
        }

        else if(instrucao == 0xA0){ //JZ, pula para o HLT se AC = 0
            uint8_t endereco = ler_memoria(PC);
            acessos_memoria++;
            PC++;
            if(Z == 1){
                PC = endereco;
            }
        }

        else if(instrucao == 0x90){ //JN, pula para HLT se AC < 0
            uint8_t endereco = ler_memoria(PC);
            acessos_memoria++;
            PC++;
            if(N == 1){
                PC = endereco;
            }
        }

        else if(instrucao == 0x50){ //AND
            uint8_t endereco = ler_memoria(PC);
            acessos_memoria++;
            PC++;
            AC = AC & ler_memoria(endereco);
            acessos_memoria++;
            atualizar_flags();
        }

        else if(instrucao == 0x40){ //OR
            uint8_t endereco = ler_memoria(PC);
            acessos_memoria++;
            PC++;
            AC = AC | ler_memoria(endereco);
            acessos_memoria++;
            atualizar_flags();
        }

        else if(instrucao == 0x60){ //NOT não usa operando (não lê endereço da memória)
            AC = ~AC;
            atualizar_flags();
        }

        else if(instrucao == 0x00){ //NOP
            //não faz nada
        }

        else if (instrucao == 0xF0){ //HLT
            executando = 0;
        } 

        else {
            printf("Instrução inválida: 0x%02X no endereço %d\n", instrucao, PC - 1);
            executando = 0;
        }

    }

    printf("\n=== Valor final dos registradores ===\n");
    if(strcmp(formato, "hex") == 0){
        printf("AC = 0x%02X\n", (uint8_t)AC);
        printf("PC = 0x%02X\n", PC);
        printf("IR atual = 0x%02X\n", IR_instrucao);
        printf("Flag N = 0x%02X\n", N);
        printf("Flag Z = 0x%02X\n", Z);
    } else {
        printf("AC = %d\n", AC);
        printf("PC = %d\n", PC);
        printf("IR atual = %d\n", IR_instrucao);
        printf("Flag N = %d\n", N);
        printf("Flag Z = %d\n", Z);
    }
    
    printf("Acessos à memória = %d\n", acessos_memoria);
    printf("Instruções lidas e executadas = %d\n", instrucoes_executadas);

    imprimir_mapa("Mapa de Memória DEPOIS da execução (0-255)", formato);

    return 0;
}