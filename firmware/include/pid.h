/**
 * pid.h — Controladores PID do Girino
 *
 * Dois controladores compartilham o mesmo núcleo PID (pid.cpp):
 *
 *   1. PID de VELOCIDADE (RPM → PWM)
 *      - Realimentação: velocidade medida (RPM)
 *      - Saída: 0 a 100% (somente sentido horário)
 *      - Usado na malha fechada de velocidade
 *
 *   2. PID de POSIÇÃO (ângulo → PWM bidirecional)
 *      - Realimentação: ângulo acumulado do encoder (graus)
 *      - Saída: -100 a +100% (positivo = horário, negativo = reverso)
 *      - Zona morta em torno do alvo evita trepidação do motor
 *
 * Unidades (velocidade):
 *   - setpoint / measurement: RPM
 *   - output: percent (0-100)
 *
 * Unidades (posição):
 *   - setpoint / measurement: graus
 *   - output: percent (-100 a +100)
 */

#ifndef PID_H
#define PID_H

#include "config.h"

// Controller states
enum PidState {
    PID_IDLE,    // not controlling (open loop / manual PWM)
    PID_RUNNING, // closed loop active
};

/**
 * Inicializa os dois controladores PID (velocidade e posição).
 */
void pidInit();

// =============================================================
// PID de velocidade
// =============================================================

/**
 * Define ganhos e setpoint do PID de velocidade.
 * @param kp Ganho proporcional (%/RPM)
 * @param ki Ganho integral (%/(RPM*s))
 * @param kd Ganho derivativo (%*s/RPM)
 * @param setpoint Alvo de velocidade em RPM
 */
void pidConfigure(float kp, float ki, float kd, float setpoint);

/**
 * Inicia o controle de velocidade em malha fechada.
 */
void pidStart();

/**
 * Para o controle de velocidade (saída vai a zero).
 */
void pidStop();

/**
 * @return true se a malha fechada de velocidade está ativa.
 */
bool pidIsRunning();

/**
 * Passo de cálculo do PID de velocidade.
 * Deve ser chamado a taxa fixa (PID_SAMPLE_MS).
 *
 * @param measurement Velocidade atual em RPM
 * @param dt Período de amostragem em segundos
 * @return Saída de controle em % (0-100, somente horário)
 */
float pidCompute(float measurement, float dt);

// --- Getters do PID de velocidade (API web) ---
float pidGetKp();
float pidGetKi();
float pidGetKd();
float pidGetSetpoint();
float pidGetOutput();

// =============================================================
// PID de posição (ângulo)
// =============================================================

/**
 * Define ganhos e alvo do PID de posição.
 * @param kp Ganho proporcional (%/grau)
 * @param ki Ganho integral (%/(grau*s))
 * @param kd Ganho derivativo (%*s/grau)
 * @param targetDeg Ângulo alvo em graus (origem = último encoderReset)
 */
void posConfigure(float kp, float ki, float kd, float targetDeg);

/**
 * Inicia o controle de posição (assume o motor, bidirecional).
 */
void posStart();

/**
 * Para o controle de posição.
 */
void posStop();

/**
 * @return true se o controle de posição está ativo.
 */
bool posIsRunning();

/**
 * Passo de cálculo do PID de posição.
 *
 * @param measurementDeg Ângulo atual em graus
 * @param dt Período de amostragem em segundos
 * @return Saída com sinal em % (-100 a +100); 0 dentro da zona morta
 */
float posCompute(float measurementDeg, float dt);

// --- Getters do PID de posição (API web) ---
float posGetKp();
float posGetKi();
float posGetKd();
float posGetTarget();
float posGetOutput();

#endif // PID_H
