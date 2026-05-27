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
 */
void encoderInit();

/**
 * Deve ser chamada no loop() para atualizar cálculos periódicos.
 * Calcula RPM a cada ENCODER_SAMPLE_MS milissegundos.
 */
void encoderUpdate();

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
 * Reseta a contagem de pulsos para zero.
 */
void encoderReset();

#endif // ENCODER_H
