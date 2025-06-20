# Montador RISC-V - OAC - Eduarda Gomes e Lucas Afonso

Este projeto é um montador simplificado para um subconjunto da arquitetura RISC-V, feito para a disciplina de Organização e Arquitetura de Computadores (OAC).

## Como compilar

No terminal, dentro da pasta do projeto, digite:

```bash
make
```

Isso vai gerar o executável chamado `montador`.

## Como usar

### Para mostrar o resultado no terminal:

```bash
./montador entrada.asm
```

### Para salvar o resultado em um arquivo:

```bash
./montador entrada.asm -o saida.txt
```

O arquivo `entrada.asm` deve conter as instruções em assembly, uma por linha.

## Instruções suportadas

- **Obrigatórias:** `lh`, `sh`, `sub`, `or`, `andi`, `srl`, `beq`
- **Extras:** `add`, `xor`, `sll`
- **Pseudo-instruções:** `nop`, `mv`
- **Imediatos:** Suporta decimal e hexadecimal (ex: `0xFF`)

## Exemplo de entrada.asm

```asm
add x1, x2, x3
andi x4, x5, 0xFF
lh x6, 8(x7)
beq x1, x2, 16
nop
mv x8, x9