#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "montador.h"

// Pega o número do registrador um exemplo é: "x5" vira 5
int numero_registrador(const char* texto_registrador) {
    if (texto_registrador[0] == 'x') return atoi(texto_registrador + 1);
    return -1;
}

// Transforma um número em binário, com o tamanho certo, como 5 bits)
char* decimal_para_binario(int valor, int tamanho) {
    char* binario = malloc(tamanho + 1);
    binario[tamanho] = '\0';
    for (int i = tamanho - 1; i >= 0; i--) {
        binario[i] = (valor & 1) ? '1' : '0';
        valor >>= 1;
    }
    return binario;
}

// Aqui é onde a linha do asm é analisada e montada
void montar_linha(const char* linha_asm, FILE* arquivo_saida) {
    char nome_instrucao[10], destino[10], origem1[10], origem2[20], linha_traduzida[256];

    // Se for mv ou nop, já converte pra addi pseudo-instrução (requisito da pontuação extra) 
    if (sscanf(linha_asm, "%9s %9[^,], %9s", nome_instrucao, destino, origem1) == 3 && strcmp(nome_instrucao, "mv") == 0) {
        snprintf(linha_traduzida, sizeof(linha_traduzida), "addi %s, %s, 0", destino, origem1);
        montar_linha(linha_traduzida, arquivo_saida);
        return;
    }
    if (sscanf(linha_asm, "%9s", nome_instrucao) == 1 && strcmp(nome_instrucao, "nop") == 0) {
        snprintf(linha_traduzida, sizeof(linha_traduzida), "addi x0, x0, 0");
        montar_linha(linha_traduzida, arquivo_saida);
        return;
    }

    // Aqui tenta montar instruções tipo-R (ex: add, sub, or, xor, sll, srl)
    if (sscanf(linha_asm, "%9s %9[^,],%9[^,],%9s", nome_instrucao, destino, origem1, origem2) == 4) {
        int reg_destino = numero_registrador(destino);
        int reg_origem1 = numero_registrador(origem1);
        int reg_origem2 = numero_registrador(origem2);
        char* opcode = "0110011";
        char* funct3 = NULL, *funct7 = NULL;
        if (strcmp(nome_instrucao, "sub") == 0) { funct3 = "000"; funct7 = "0100000"; }
        else if (strcmp(nome_instrucao, "or") == 0) { funct3 = "110"; funct7 = "0000000"; }
        else if (strcmp(nome_instrucao, "srl") == 0) { funct3 = "101"; funct7 = "0000000"; }
        else if (strcmp(nome_instrucao, "add") == 0) { funct3 = "000"; funct7 = "0000000"; }
        else if (strcmp(nome_instrucao, "xor") == 0) { funct3 = "100"; funct7 = "0000000"; }
        else if (strcmp(nome_instrucao, "sll") == 0) { funct3 = "001"; funct7 = "0000000"; }
        if (funct7) {
            char* bin_destino = decimal_para_binario(reg_destino, 5);
            char* bin_origem1 = decimal_para_binario(reg_origem1, 5);
            char* bin_origem2 = decimal_para_binario(reg_origem2, 5);
            fprintf(arquivo_saida, "%s%s%s%s%s%s\n", funct7, bin_origem2, bin_origem1, funct3, bin_destino, opcode);
            free(bin_destino); free(bin_origem1); free(bin_origem2);
            return;
        }
    }

    // Aqui tenta montar instruções tipo-I (andi, addi, beq)
    if (sscanf(linha_asm, "%9s %9[^,],%9[^,],%19s", nome_instrucao, destino, origem1, origem2) == 4) {
        int reg_destino = numero_registrador(destino);
        int reg_origem1 = numero_registrador(origem1);
        int valor_imediato = strtol(origem2, NULL, 0); // aceita decimal e hex
        char* opcode = NULL, *funct3 = NULL;
        if (strcmp(nome_instrucao, "andi") == 0) { opcode = "0010011"; funct3 = "111"; }
        else if (strcmp(nome_instrucao, "addi") == 0) { opcode = "0010011"; funct3 = "000"; }
        if (opcode) {
            char* bin_imediato = decimal_para_binario(valor_imediato, 12);
            char* bin_origem1 = decimal_para_binario(reg_origem1, 5);
            char* bin_destino = decimal_para_binario(reg_destino, 5);
            fprintf(arquivo_saida, "%s%s%s%s%s\n", bin_imediato, bin_origem1, funct3, bin_destino, opcode);
            free(bin_imediato); free(bin_origem1); free(bin_destino);
            return;
        }
        // beq é um pouco diferente, tem que embaralhar os bits do imediato
        if (strcmp(nome_instrucao, "beq") == 0) {
            opcode = "1100011"; funct3 = "000";
            int reg1 = numero_registrador(destino), reg2 = numero_registrador(origem1);
            int im12 = (valor_imediato >> 12) & 1, im10_5 = (valor_imediato >> 5) & 0x3F, im4_1 = (valor_imediato >> 1) & 0xF, im11 = (valor_imediato >> 11) & 1;
            char* b12 = decimal_para_binario(im12, 1), *b10_5 = decimal_para_binario(im10_5, 6), *b4_1 = decimal_para_binario(im4_1, 4), *b11 = decimal_para_binario(im11, 1);
            char* bin_reg1 = decimal_para_binario(reg1, 5), *bin_reg2 = decimal_para_binario(reg2, 5);
            fprintf(arquivo_saida, "%s%s%s%s%s%s%s%s\n", b12, b10_5, bin_reg2, bin_reg1, funct3, b4_1, b11, opcode);
            free(b12); free(b10_5); free(b4_1); free(b11); free(bin_reg1); free(bin_reg2);
            return;
        }
    }

    // Aqui monta lh (load) e sh (store)
    if (sscanf(linha_asm, "%9s %9[^,],%19[^(](%9[^)])", nome_instrucao, destino, origem2, origem1) == 4) {
        int valor_imediato = strtol(origem2, NULL, 0);
        if (strcmp(nome_instrucao, "lh") == 0) {
            int reg_destino = numero_registrador(destino), reg_base = numero_registrador(origem1);
            char* opcode = "0000011", *funct3 = "001";
            char* bin_imediato = decimal_para_binario(valor_imediato, 12);
            char* bin_base = decimal_para_binario(reg_base, 5);
            char* bin_destino = decimal_para_binario(reg_destino, 5);
            fprintf(arquivo_saida, "%s%s%s%s%s\n", bin_imediato, bin_base, funct3, bin_destino, opcode);
            free(bin_imediato); free(bin_base); free(bin_destino);
            return;
        }
        if (strcmp(nome_instrucao, "sh") == 0) {
            int reg_origem = numero_registrador(destino), reg_base = numero_registrador(origem1);
            char* opcode = "0100011", *funct3 = "001";
            int im11_5 = (valor_imediato >> 5) & 0x7F, im4_0 = valor_imediato & 0x1F;
            char* bin11_5 = decimal_para_binario(im11_5, 7), *bin4_0 = decimal_para_binario(im4_0, 5);
            char* bin_base = decimal_para_binario(reg_base, 5), *bin_origem = decimal_para_binario(reg_origem, 5);
            fprintf(arquivo_saida, "%s%s%s%s%s%s\n", bin11_5, bin_origem, bin_base, funct3, bin4_0, opcode);
            free(bin11_5); free(bin4_0); free(bin_base); free(bin_origem);
            return;
        }
    }

    // Se não reconheceu, mostra erro simples
    fprintf(arquivo_saida, "Linha não reconhecida: %s\n", linha_asm);
}

// Lê o arquivo linha por linha e chama a montagem
int montar_arquivo(const char *arquivo_entrada, const char *arquivo_saida) {
    FILE *arquivo = fopen(arquivo_entrada, "r");
    if (!arquivo) {
        printf("Erro ao abrir o arquivo de entrada.\n");
        return 1;
    }
    FILE *saida = arquivo_saida ? fopen(arquivo_saida, "w") : stdout;
    if (!saida) {
        printf("Erro ao abrir o arquivo de saída.\n");
        fclose(arquivo);
        return 1;
    }
    char linha_asm[256];
    while (fgets(linha_asm, sizeof(linha_asm), arquivo)) {
        linha_asm[strcspn(linha_asm, "\r\n")] = 0;
        char* ptr = linha_asm;
        while(isspace((unsigned char)*ptr)) ptr++;
        if(*ptr == '\0' || *ptr == '#') continue;
        montar_linha(ptr, saida);
    }
    fclose(arquivo);
    if (saida != stdout) fclose(saida);
    return 0;
}

