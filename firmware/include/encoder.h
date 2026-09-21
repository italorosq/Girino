/**
 * encoder.h — Leitura de encoder incremental LPD3806-600BM
 *
 * Funções para inicializar e ler o encoder via interrupções GPIO.
 * O encoder gera pulsos quadratura (canais A e B) que permitem
 * determinar posição e velocidade angular.
 */

#ifndef ENCODER_H
#define ENCODER_H

#include "config.h"

/**
 * Inicializa os pinos do encoder e configura interrupções.
 * Usa interrupções no canal A para contagem de pulsos.
 *
 * ATENÇÃO: o LPD3806 tem saída em coletor aberto — a leitura exige
 * resistores de pull-up externos de 4.7k para 3.3V (não 5V).
 */
void encoderInit();

/**
 * Deve ser chamada no loop() para atualizar cálculos periódicos.
 * Calcula RPM a cada ENCODER_SAMPLE_MS milissegundos.
 */
void encoderUpdate();

/**
 * Feedback didático do encoder (chamar no loop):
 *  - LED onboard pisca enquanto chegam pulsos (conferência da fiação)
 *  - Telemetria periódica no serial: pulsos, pulsos/s, RPM e ângulo
 *    (intervalo definido por ENCODER_DEBUG_INTERVAL_MS em config.h)
 */
void encoderUpdateFeedback();

/**
 * Retorna o número total de pulsos desde a inicialização.
 * @return Contagem de pulsos (positivo = horário, negativo = anti-horário)
 */
long encoderGetPulses();

/**
 * Retorna a velocidade angular atual em RPM.
 * @return Velocidade em revoluções por minuto
 */
float encoderGetRPM();

/**
 * Retorna o ângulo acumulado em graus (com sinal).
 * Positivo = sentido horário, negativo = anti-horário.
 * A origem (0°) é definida por encoderReset().
 * @return Ângulo em graus
 */
float encoderGetAngle();

/**
 * Reseta a contagem de pulsos para zero (define a origem do ângulo).
 */
void encoderReset();

#endif // ENCODER_H
