#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "montador.h"

/*
 * Trabalho de OAC - Montador RISC-V 
 * Integrantes do grupo: Eduarda Gomes Camilo e Lucas Afonso 
 *Para nos organizarmos melhor no desenvolvimento do trabalho, indicamos com comentários os requisitos
 *de avaliação que o professor indicou na descrição do trabalho e os extras.
 */


int main(int quantidade_args, char *args[]) {
    // Checa se os argumentos estão certos
    if (quantidade_args < 2 || quantidade_args == 3 || quantidade_args > 4) {
        printf("Uso: %s <arquivo_entrada.asm> [-o <arquivo_saida>]\n", args[0]);
        return 1;
    }

    const char *arquivo_entrada = args[1];
    const char *arquivo_saida = NULL;

    // Se o usuário quiser salvar em arquivo, pega o nome do arquivo de saída
    if (quantidade_args == 4) {
        if (strcmp(args[2], "-o") == 0) {
            arquivo_saida = args[3];
        } else {
            printf("Argumento '%s' inválido. Use '-o' para especificar o arquivo de saída.\n", args[2]);
            return 1;
        }
    }

    // Chama a função que faz a montagem de verdade
    int resultado = montar_arquivo(arquivo_entrada, arquivo_saida);

    if (resultado != 0) {
        printf("Erro ao montar o arquivo '%s'.\n", arquivo_entrada);
        return 1;
    }

    printf("Montagem de '%s' concluída.\n", arquivo_entrada);
    if (arquivo_saida) {
        printf("Resultado salvo em: '%s'\n", arquivo_saida);
    }

    return 0;
}