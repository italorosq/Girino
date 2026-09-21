/**
 * pid.cpp — Núcleo PID compartilhado (velocidade e posição)
 *
 * Educational notes (for the students):
 *
 * The controller uses the standard positional form, discretized:
 *
 *   u[k] = Kp * e[k] + Ki * integral(e) + Kd * derivative
 *
 * Two practical details matter for real motors:
 *
 * 1. ANTI-WINDUP: when the motor saturates (e.g. 100% PWM during a big
 *    step), the integral term would keep growing ("windup") and cause a
 *    huge overshoot when the setpoint is finally reached. We handle it
 *    two ways: (a) conditional integration — the integral is frozen while
 *    the output is saturated in the same direction of the error — and
 *    (b) a hard clamp on the integral as a last resort.
 *
 * 2. DERIVATIVE ON MEASUREMENT: differentiating the error causes a
 *    "derivative kick" every time the setpoint changes. Differentiating
 *    the measurement instead (with a negative sign) gives the same
 *    regulation effect without the kick.
 *
 * O mesmo núcleo é usado por dois controladores:
 *   - velocidade: saída 0..+100% (somente horário)
 *   - posição:    saída -100..+100% (bidirecional)
 */

#include <Arduino.h>
#include "pid.h"
#include "motor_control.h"

// --- Núcleo genérico de um controlador PID ---
struct PidCore {
    PidState state;
    float kp, ki, kd;
    float setpoint;
    float integral;
    float prevMeasurement;
    float output;
    float outMin, outMax;
    bool firstSample;  // primeiro passo após start: sem derivada
};

static PidCore speedPid;
static PidCore posPid;

static void coreInit(PidCore& c, float outMin, float outMax) {
    c.state = PID_IDLE;
    c.kp = c.ki = c.kd = 0.0;
    c.setpoint = 0.0;
    c.integral = 0.0;
    c.prevMeasurement = 0.0;
    c.output = 0.0;
    c.outMin = outMin;
    c.outMax = outMax;
    c.firstSample = true;
}

static void coreReset(PidCore& c) {
    c.integral = 0.0;
    c.prevMeasurement = 0.0;
    c.output = 0.0;
    c.firstSample = true;
}

static float coreCompute(PidCore& c, float measurement, float dt) {
    if (dt <= 0.0) return c.output;

    // No primeiro passo após o start, memoriza a medição atual para
    // que a derivada não dê um "salto" (0 -> medição real).
    if (c.firstSample) {
        c.prevMeasurement = measurement;
        c.firstSample = false;
    }

    float error = c.setpoint - measurement;

    // --- Integral with anti-windup ---
    float newIntegral = c.integral + error * dt;

    // --- Derivative on measurement ---
    float derivative = -(measurement - c.prevMeasurement) / dt;

    // --- Raw output ---
    float rawOutput = c.kp * error + c.ki * newIntegral + c.kd * derivative;

    // --- Saturation / anti-windup (conditional integration) ---
    bool saturatedHigh = rawOutput > c.outMax;
    bool saturatedLow  = rawOutput < c.outMin;
    bool pushingHigh   = error > 0.0;
    bool pushingLow    = error < 0.0;

    if ((saturatedHigh && pushingHigh) || (saturatedLow && pushingLow)) {
        // Do not integrate while pushing further into saturation
        newIntegral = c.integral;
        rawOutput = c.kp * error + c.ki * newIntegral + c.kd * derivative;
    }

    // Hard clamp on the integral as a last resort
    float integralMin = c.outMin / (c.ki > 0.0 ? c.ki : 1.0);
    float integralMax = c.outMax / (c.ki > 0.0 ? c.ki : 1.0);
    if (newIntegral > integralMax) newIntegral = integralMax;
    if (newIntegral < integralMin) newIntegral = integralMin;

    c.integral = newIntegral;
    c.prevMeasurement = measurement;

    // Final clamp
    if (rawOutput > c.outMax) rawOutput = c.outMax;
    if (rawOutput < c.outMin) rawOutput = c.outMin;
    c.output = rawOutput;

    return c.output;
}

void pidInit() {
    coreInit(speedPid, PID_OUTPUT_MIN, PID_OUTPUT_MAX);
    coreInit(posPid, POS_OUTPUT_MIN, POS_OUTPUT_MAX);
    Serial.println("[PID] Inicializado");
    Serial.println("[Pos] Controle de posicao inicializado (angulo)");
}

// =============================================================
// PID de velocidade
// =============================================================

void pidConfigure(float kp, float ki, float kd, float setpoint) {
    speedPid.kp = kp;
    speedPid.ki = ki;
    speedPid.kd = kd;
    speedPid.setpoint = setpoint;

    coreReset(speedPid);

    Serial.print("[PID] Config: Kp=");
    Serial.print(kp);
    Serial.print(" Ki=");
    Serial.print(ki);
    Serial.print(" Kd=");
    Serial.print(kd);
    Serial.print(" SP=");
    Serial.println(setpoint);
}

void pidStart() {
    coreReset(speedPid);
    speedPid.state = PID_RUNNING;
    Serial.println("[PID] Controle em malha fechada iniciado");
}

void pidStop() {
    speedPid.state = PID_IDLE;
    coreReset(speedPid);
    Serial.println("[PID] Controle parado");
}

bool pidIsRunning() {
    return speedPid.state == PID_RUNNING;
}

float pidCompute(float measurement, float dt) {
    if (speedPid.state != PID_RUNNING) return 0.0;
    return coreCompute(speedPid, measurement, dt);
}

float pidGetKp() { return speedPid.kp; }
float pidGetKi() { return speedPid.ki; }
float pidGetKd() { return speedPid.kd; }
float pidGetSetpoint() { return speedPid.setpoint; }
float pidGetOutput() { return speedPid.output; }

// =============================================================
// PID de posição (ângulo)
// =============================================================

void posConfigure(float kp, float ki, float kd, float targetDeg) {
    posPid.kp = kp;
    posPid.ki = ki;
    posPid.kd = kd;
    posPid.setpoint = targetDeg;

    coreReset(posPid);

    Serial.print("[Pos] Config: Kp=");
    Serial.print(kp);
    Serial.print(" Ki=");
    Serial.print(ki);
    Serial.print(" Kd=");
    Serial.print(kd);
    Serial.print(" Alvo=");
    Serial.print(targetDeg);
    Serial.println(" graus");
}

void posStart() {
    coreReset(posPid);
    posPid.state = PID_RUNNING;
    Serial.println("[Pos] Controle de posicao iniciado");
}

void posStop() {
    posPid.state = PID_IDLE;
    coreReset(posPid);
    Serial.println("[Pos] Controle de posicao parado");
}

bool posIsRunning() {
    return posPid.state == PID_RUNNING;
}

float posCompute(float measurementDeg, float dt) {
    if (posPid.state != PID_RUNNING) return 0.0;

    // Zona morta: erro pequeno -> saída zero (evita trepidação).
    // O integrador é mantido para não perder o esforço acumulado.
    float error = posPid.setpoint - measurementDeg;
    if (fabs(error) <= POS_DEADBAND_DEG) {
        posPid.output = 0.0;
        posPid.prevMeasurement = measurementDeg;
        return 0.0;
    }

    // NOTA DIDÁTICA: neste motor a zona morta é alta (~60% de duty), o
    // que torna o controle de posição "bang-bang" (cada acionamento
    // mínimo move ~230 RPM). Com um driver adequado (zona morta ~10-20%)
    // o mesmo PID converge suavemente.
    return coreCompute(posPid, measurementDeg, dt);
}

float posGetKp() { return posPid.kp; }
float posGetKi() { return posPid.ki; }
float posGetKd() { return posPid.kd; }
float posGetTarget() { return posPid.setpoint; }
float posGetOutput() { return posPid.output; }
