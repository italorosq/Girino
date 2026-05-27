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

#endif // MOTOR_CONTROL_H
