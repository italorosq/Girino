# Exemplo 01 — Controle Básico de Motor

Introdução ao controle PWM de um motor DC via ESP8266.

## Objetivos de Aprendizagem

- Entender o conceito de PWM (Pulse Width Modulation)
- Controlar a velocidade de um motor DC
- Controlar a direção de rotação

## Conceitos

### PWM (Modulação por Largura de Pulso)

O PWM controla a potência entregue ao motor variando o tempo que o sinal fica em nível alto (duty cycle).

- **Duty cycle 0%** → Motor parado
- **Duty cycle 50%** → Motor a meia velocidade
- **Duty cycle 100%** → Motor a velocidade máxima

### Controle de Direção

Usando dois pinos digitais (IN1 e IN2):

| IN1 | IN2 | Resultado |
|-----|-----|-----------|
| HIGH | LOW | Sentido horário |
| LOW | HIGH | Sentido anti-horário |
| LOW | LOW | Motor livre (parado) |
| HIGH | HIGH | Frenagem (freio) |

## Código

> Em breve — será adicionado com a implementação.

## Exercícios Propostos

1. Fazer o motor acelerar gradualmente de 0% a 100%
2. Alternar a direção a cada 3 segundos
3. Criar um "rampa" de aceleração suave
