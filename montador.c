#include "montador.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * Converte uma string de registrador (ex: "x10") para seu número inteiro (10).
 * Retorna -1 se o formato for inválido.
 */
int get_register_num(const char* reg_str) {
    if (reg_str[0] == 'x') {
        return atoi(reg_str + 1);
    }
    // Futuramente, pode-se adicionar suporte para nomes como "sp", "ra", etc.
    return -1; // Retorna -1 se o formato for inválido
}

/*
 * Converte um número decimal para uma string binária de 'len' bits.
 * Aloca memória para a string, que deve ser liberada posteriormente.
 */
char* dec_to_binary(int n, int len) {
    char* bin = (char*)malloc(sizeof(char) * (len + 1));
    if (!bin) return NULL;
    bin[len] = '\0';
    for (int i = len - 1; i >= 0; i--) {
        bin[i] = (n & 1) ? '1' : '0';
        n >>= 1;
    }
    return bin;
}

/*
 * Função central que processa uma única linha de assembly.
 * A estratégia é tentar analisar a linha com os diferentes formatos de instrução
 * (Pseudo-instruções, Tipo-R, Tipo-I, etc.) em sequência. Quando um formato
 * é reconhecido, a instrução é montada e a função retorna.
 */
void processar_linha(const char* linha, FILE* stream_saida) {
    char instrucao[10];
    char p1[10], p2[10], p3[20];
    char linha_traduzida[256];

    // --- Passo 1: Checa e traduz Pseudo-instruções ---
    // Reconhece 'mv' e 'nop' e as converte para suas instruções 'addi' reais.
    // A função então chama a si mesma (recursão) com a instrução traduzida.
    if (sscanf(linha, "%9s %9[^,], %9s", instrucao, p1, p2) == 3 && strcmp(instrucao, "mv") == 0) {
        snprintf(linha_traduzida, sizeof(linha_traduzida), "addi %s, %s, 0", p1, p2);
        processar_linha(linha_traduzida, stream_saida);
        return;
    }
    if (sscanf(linha, "%9s", instrucao) == 1 && strcmp(instrucao, "nop") == 0) {
        snprintf(linha_traduzida, sizeof(linha_traduzida), "addi x0, x0, 0");
        processar_linha(linha_traduzida, stream_saida);
        return;
    }

    // --- Passo 2: Se não for pseudo-instrução, tenta os formatos reais ---

    // Formato Tipo-R: instrucao rd, rs1, rs2
    if (sscanf(linha, "%9s %9[^,],%9[^,],%9s", instrucao, p1, p2, p3) == 4) {
        int rd = get_register_num(p1);
        int rs1 = get_register_num(p2);
        int rs2 = get_register_num(p3);

        char* opcode_bin = "0110011";
        char* funct3_bin = NULL;
        char* funct7_bin = NULL;
        
        // Mapeia o nome da instrução para seus códigos de função (funct3 e funct7)
        if (strcmp(instrucao, "sub") == 0) {
            funct3_bin = "000"; funct7_bin = "0100000";
        } else if (strcmp(instrucao, "or") == 0) {
            funct3_bin = "110"; funct7_bin = "0000000";
        } else if (strcmp(instrucao, "srl") == 0) {
            funct3_bin = "101"; funct7_bin = "0000000";
        } else if (strcmp(instrucao, "add") == 0) {
            funct3_bin = "000"; funct7_bin = "0000000";
        } else if (strcmp(instrucao, "xor") == 0) {
            funct3_bin = "100"; funct7_bin = "0000000";
        } else if (strcmp(instrucao, "sll") == 0) {
            funct3_bin = "001"; funct7_bin = "0000000";
        }
        
        if (funct7_bin) { // Se a instrução foi mapeada com sucesso
            char* rd_bin = dec_to_binary(rd, 5);
            char* rs1_bin = dec_to_binary(rs1, 5);
            char* rs2_bin = dec_to_binary(rs2, 5);
            
            // Monta a string final de acordo com o formato Tipo-R
            fprintf(stream_saida, "%s%s%s%s%s%s\n", funct7_bin, rs2_bin, rs1_bin, funct3_bin, rd_bin, opcode_bin);
            
            free(rd_bin); free(rs1_bin); free(rs2_bin);
            return;
        }
    }

    // Formato Tipo-I (Lógica/Branch): instrucao rd, rs1, imm
    if (sscanf(linha, "%9s %9[^,],%9[^,],%19s", instrucao, p1, p2, p3) == 4) {
        int rd = get_register_num(p1);
        int rs1 = get_register_num(p2);
        // Usa strtol para suportar tanto decimal quanto hexadecimal (0x)
        int imm = strtol(p3, NULL, 0);

        char* opcode_bin = NULL;
        char* funct3_bin = NULL;

        // Mapeia a instrução para seu opcode e funct3
        if (strcmp(instrucao, "andi") == 0) {
            opcode_bin = "0010011"; funct3_bin = "111";
        } else if (strcmp(instrucao, "addi") == 0) {
            opcode_bin = "0010011"; funct3_bin = "000";
        }
        
        if (opcode_bin) {
            char* imm_bin = dec_to_binary(imm, 12);
            char* rs1_bin = dec_to_binary(rs1, 5);
            char* rd_bin = dec_to_binary(rd, 5);
            
            // Monta a string final de acordo com o formato Tipo-I
            fprintf(stream_saida, "%s%s%s%s%s\n", imm_bin, rs1_bin, funct3_bin, rd_bin, opcode_bin);

            free(imm_bin); free(rs1_bin); free(rd_bin);
            return;
        }

        // Tratamento do Tipo-B (beq)
        if (strcmp(instrucao, "beq") == 0) {
            opcode_bin = "1100011"; 
            funct3_bin = "000";

            int rs1 = get_register_num(p1);
            int rs2 = get_register_num(p2);

            // O imediato do tipo B é "embaralhado" em 4 partes.
            // Aqui, extraímos cada parte usando máscaras e deslocamentos de bits.
            int imm_12   = (imm >> 12) & 1;
            int imm_10_5 = (imm >> 5)  & 0x3F;
            int imm_4_1  = (imm >> 1)  & 0xF;
            int imm_11   = (imm >> 11) & 1;

            char* imm12_bin   = dec_to_binary(imm_12, 1);
            char* imm10_5_bin = dec_to_binary(imm_10_5, 6);
            char* imm4_1_bin  = dec_to_binary(imm_4_1, 4);
            char* imm11_bin   = dec_to_binary(imm_11, 1);
            char* rs1_bin     = dec_to_binary(rs1, 5);
            char* rs2_bin     = dec_to_binary(rs2, 5);

            // Monta a string final de acordo com o complexo formato Tipo-B
            fprintf(stream_saida, "%s%s%s%s%s%s%s%s\n",
                imm12_bin, imm10_5_bin, rs2_bin, rs1_bin, funct3_bin, imm4_1_bin, imm11_bin, opcode_bin);

            free(imm12_bin); free(imm10_5_bin); free(imm4_1_bin); free(imm11_bin);
            free(rs1_bin); free(rs2_bin);
            return;
        }
    }

    // Formato Tipo-I (Load) e Tipo-S (Store): instrucao reg, offset(base_reg)
    // sscanf complexo para capturar os 4 componentes: instrução, reg, offset, base_reg
    if (sscanf(linha, "%9s %9[^,],%19[^(](%9[^)])", instrucao, p1, p3, p2) == 4) {
        // Usa strtol para suportar tanto decimal quanto hexadecimal (0x)
        int imm = strtol(p3, NULL, 0);
        char* opcode_bin = NULL;
        char* funct3_bin = NULL;

        // Tratamento do Tipo I (lh)
        if (strcmp(instrucao, "lh") == 0) {
            opcode_bin = "0000011";
            funct3_bin = "001";

            int rd = get_register_num(p1);
            int rs1 = get_register_num(p2);

            char* imm_bin = dec_to_binary(imm, 12);
            char* rs1_bin = dec_to_binary(rs1, 5);
            char* rd_bin = dec_to_binary(rd, 5);
            
            // Monta a string final de acordo com o formato Tipo-I (Load)
            fprintf(stream_saida, "%s%s%s%s%s\n", imm_bin, rs1_bin, funct3_bin, rd_bin, opcode_bin);

            free(imm_bin); free(rs1_bin); free(rd_bin);
            return;
        }

        // Tratamento do Tipo S (sh)
        if (strcmp(instrucao, "sh") == 0) {
            opcode_bin = "0100011";
            funct3_bin = "001";

            // Note que no store, o primeiro operando é o registrador a ser salvo (rs2)
            int rs2 = get_register_num(p1);
            int rs1 = get_register_num(p2);

            // Divide o imediato de 12 bits para o formato Tipo-S
            int imm_11_5 = (imm >> 5) & 0x7F;
            int imm_4_0  = imm & 0x1F;

            char* imm11_5_bin = dec_to_binary(imm_11_5, 7);
            char* imm4_0_bin  = dec_to_binary(imm_4_0, 5);
            char* rs1_bin     = dec_to_binary(rs1, 5);
            char* rs2_bin     = dec_to_binary(rs2, 5);

            // Monta a string final de acordo com o formato Tipo-S
            fprintf(stream_saida, "%s%s%s%s%s%s\n", imm11_5_bin, rs2_bin, rs1_bin, funct3_bin, imm4_0_bin, opcode_bin);

            free(imm11_5_bin); free(imm4_0_bin); free(rs1_bin); free(rs2_bin);
            return;
        }
    }
    
    // Se nenhum dos 'if' acima conseguiu processar a linha, ela é inválida.
    fprintf(stream_saida, "ERRO: Instrução '%s' não reconhecida ou formato inválido.\n", linha);
}

/*
 * Abre os arquivos de entrada e saída e lê o arquivo de entrada linha por linha.
 */
int montar_arquivo(const char *input_file, const char *output_file) {
    FILE *fin = fopen(input_file, "r");
    if (!fin) {
        printf("Erro ao abrir o arquivo de entrada '%s'.\n", input_file);
        return 1;
    }

    // Define o stream de saída: ou um arquivo, ou o terminal (stdout)
    FILE *fout = output_file ? fopen(output_file, "w") : stdout;
    if (!fout) {
        printf("Erro ao criar o arquivo de saída '%s'.\n", output_file);
        fclose(fin);
        return 1;
    }

    char linha[256];
    while (fgets(linha, sizeof(linha), fin)) {
        // Limpa a linha, removendo quebras de linha de Windows (\r\n) e Linux (\n)
        linha[strcspn(linha, "\r\n")] = 0;
        
        // Avança o ponteiro para ignorar espaços/tabs no início da linha.
        char* ptr = linha;
        while(isspace((unsigned char)*ptr)) ptr++;
        
        // Pula o processamento de linhas vazias ou de comentário.
        if(*ptr == '\0' || *ptr == '#') {
            continue;
        }
        
        // Envia a linha limpa para a função de processamento.
        processar_linha(ptr, fout);
    }

    fclose(fin);
    if (fout != stdout) {
        fclose(fout);
    }
    return 0;
}

// Comente a OPÇÃO 2
// const char *input_file = "entrada.asm";
// const char *output_file = "saida.txt";

// Descomente a OPÇÃO 1 para imprimir no terminal
const char *input_file = "entrada.asm";
const char *output_file = NULL; // NULL significa "imprimir no terminal" 