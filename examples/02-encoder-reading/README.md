# Exemplo 02 — Leitura de Encoder

Introdução à leitura de encoder incremental LPD3806-600BM.

## Objetivos de Aprendizagem

- Entender o princípio de encoder incremental
- Ler pulsos via interrupção GPIO
- Calcular velocidade angular (RPM)

## Conceitos

### Encoder Incremental

O LPD3806-600BM gera 600 pulsos por revolução em dois canais (A e B) defasados em 90°:

- **Canal A sozinho** → Contagem de pulsos (velocidade)
- **Canais A+B** → Determinação de direção (quadratura)

### Interrupções

O ESP8266 suporta interrupções em GPIOs. Usamos interrupção no canal A e lemos o canal B na ISR para determinar a direção.

## Código

> Em breve — será adicionado com a implementação.

## Exercícios Propostos

1. Contar o número de voltas completas do motor
2. Medir a velocidade em RPM para diferentes níveis de PWM
3. Calcular a distância percorrida (se o motor aciona uma roda)
