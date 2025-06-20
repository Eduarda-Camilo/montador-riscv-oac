#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "montador.h"

/*
 * Exemplos usados no terminal para teste:
 * ./montador entrada.asm
 * ./montador entrada.asm -o saida.bin
 *
 */
int main(int argc, char *argv[]) {
    // Valida a quantidade de argumentos para evitar erros de uso.
    if (argc < 2 || argc == 3 || argc > 4) {
        fprintf(stderr, "Uso: %s <arquivo_entrada.asm> [-o <arquivo_saida>]\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = NULL;

    // Verifica se a flag de saída '-o' foi fornecida corretamente.
    if (argc == 4) {
        if (strcmp(argv[2], "-o") == 0) {
            output_file = argv[3];
        } else {
            fprintf(stderr, "ERRO: Argumento '%s' inválido. Use '-o' para especificar o arquivo de saída.\n", argv[2]);
            return 1;
        }
    }

    // Chama a função principal do montador.
    int status = montar_arquivo(input_file, output_file);

    if (status != 0) {
        fprintf(stderr, "ERRO: Falha ao montar o arquivo '%s'.\n", input_file);
        return 1;
    }

    printf("Montagem de '%s' concluída com sucesso.\n", input_file);
    if (output_file) {
        printf("Resultado salvo em: '%s'\n", output_file);
    }

    return 0;
}