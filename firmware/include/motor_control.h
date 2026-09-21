/**
 * motor_control.h — Controle PWM de motor DC
 *
 * Funções para inicializar e controlar o motor DC
 * via PWM e pinos de direção.
 */

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include "config.h"

/**
 * Inicializa os pinos do motor (PWM + direção).
 */
void motorControlInit();

/**
 * Define a velocidade do motor (0 a MOTOR_MAX_SPEED %).
 * @param speed Percentual de velocidade (0-100)
 */
void motorSetSpeed(int speed);

/**
 * Define a direção do motor.
 * @param direction MOTOR_DIR_FORWARD, MOTOR_DIR_REVERSE ou MOTOR_DIR_STOP
 */
void motorSetDirection(int direction);

/**
 * Retorna a velocidade atual do motor.
 * @return Percentual de velocidade (0-100)
 */
int motorGetSpeed();

/**
 * Retorna a direção atual do motor.
 * @return MOTOR_DIR_FORWARD, MOTOR_DIR_REVERSE ou MOTOR_DIR_STOP
 */
int motorGetDirection();

/**
 * @return true se a fiação motor/encoder foi detectada invertida
 *         (e está sendo corrigida em software).
 */
bool motorDirInverted();

/**
 * Detecta o sentido físico da fiação: aplica um pulso curto de teste no
 * primeiro movimento após o boot e observa a contagem do encoder. Se a
 * contagem for negativa, o mapa de direção é invertido em software —
 * assim FORWARD ("horário") sempre aumenta o ângulo medido, qualquer
 * que seja a fiação do motor ou dos canais A/B do encoder.
 *
 * É chamada automaticamente pelo primeiro motorSetSpeed(>0).
 */
void motorCalibrateDirection();

#endif // MOTOR_CONTROL_H
