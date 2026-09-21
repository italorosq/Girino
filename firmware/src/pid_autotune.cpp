/**
 * pid_autotune.cpp — Relay feedback experiment implementation
 *
 * The relay alternates between (bias + d) and (bias - d) each time the
 * measured speed crosses the setpoint. The BIAS keeps the motor above
 * its dead zone: a DC motor needs a minimum PWM to overcome static
 * friction — without a bias, a 20% relay alternated with 0% would just
 * buzz without turning (measured dead zone on this bench: ~80%).
 *
 * Timing data is collected as:
 *   - full periods: time between every second setpoint crossing
 *   - amplitude: (max(RPM) - min(RPM)) / 2 measured during the cycle
 *
 * The first half-cycle is skipped (spin-up transient is not part of
 * the limit cycle).
 */

#include <Arduino.h>
#include "pid_autotune.h"

// --- Configuration (runtime, set at autotuneStart) ---
static float relayAmp = 0.0;       // d in % PWM (nominal amplitude)
static float relayHigh = 0.0;      // output while below setpoint (% PWM)
static float relayLow = 0.0;       // output while at/above setpoint (% PWM)
static float relayStep = 0.0;      // effective step d = (high - low) / 2
static int requiredPeriods = 3;    // number of full periods to average
static float setpoint = 0.0;       // RPM

// --- State machine ---
static AutotuneState state = AUTOTUNE_IDLE;
static unsigned long startTime = 0;     // ms (experiment start)
static unsigned long lastCrossTime = 0; // ms (last crossing, mesmo rejeitado — debounce)
static unsigned long lastAcceptedCrossTime = 0; // ms (último cruzamento aceito)
static unsigned long lastPeriodAnchor = 0;      // ms (cruzamento par — âncora do período)

// --- Oscillation data ---
static int crossCount = 0;              // total sign flips of the error
static unsigned long periodSum = 0;     // sum of full periods (ms)
static int periodCount = 0;             // number of full periods collected
static float rpmMin = 0.0;              // min RPM of current limit cycle
static float rpmMax = 0.0;              // max RPM of current limit cycle

// --- Current relay output ---
static float relayOutput = 0.0;

// --- Previous error (sign flip detection) ---
static float prevError = 1.0; // starts positive (motor stopped, SP > 0)

// --- Results ---
static float resultKu = 0.0;
static float resultTu = 0.0;

void autotuneInit() {
    state = AUTOTUNE_IDLE;
    relayOutput = 0.0;
    resultKu = 0.0;
    resultTu = 0.0;
    Serial.println("[AutoTune] Inicializado");
}

void autotuneStart(float newRelayAmp, float bias, int cycles, float newSetpoint) {
    relayAmp = newRelayAmp;
    requiredPeriods = (cycles < 1) ? 3 : cycles;
    setpoint = newSetpoint;

    // Relay com pedestal: estados alto/baixo em torno do bias.
    // O bias precisa vencer a zona morta do motor para que a oscilação
    // aconteça de verdade (o motor não pode parar no estado baixo).
    relayHigh = bias + relayAmp;
    relayLow = bias - relayAmp;
    if (relayHigh > 100.0) relayHigh = 100.0;
    if (relayLow < 0.0) relayLow = 0.0;
    // Amplitude efetiva do degrau (usada no cálculo de Ku)
    relayStep = (relayHigh - relayLow) / 2.0;

    startTime = millis();
    lastCrossTime = startTime;
    lastAcceptedCrossTime = startTime;
    lastPeriodAnchor = startTime;
    crossCount = 0;
    periodSum = 0;
    periodCount = 0;
    rpmMin = 0.0;
    rpmMax = 0.0;
    relayOutput = 0.0;
    resultKu = 0.0;
    resultTu = 0.0;
    prevError = 1.0;

    state = AUTOTUNE_RUNNING;
    Serial.print("[AutoTune] Iniciado: d=");
    Serial.print(relayAmp);
    Serial.print("% bias=");
    Serial.print(bias);
    Serial.print("% (saida ");
    Serial.print(relayLow);
    Serial.print("/");
    Serial.print(relayHigh);
    Serial.print("%) ciclos=");
    Serial.print(requiredPeriods);
    Serial.print(" SP=");
    Serial.println(setpoint);
}

void autotuneCancel() {
    if (state == AUTOTUNE_RUNNING) {
        Serial.println("[AutoTune] Experimento cancelado");
    }
    state = AUTOTUNE_IDLE;
    relayOutput = 0.0;
}

void autotuneCompute(float measurement, float dt) {
    if (state != AUTOTUNE_RUNNING) {
        relayOutput = 0.0;
        return;
    }

    unsigned long now = millis();

    // O relay só aciona em um sentido (horário) — velocidade negativa
    // é ruído de medição e não deve poluir o envelope da oscilação.
    if (measurement < 0.0) measurement = 0.0;

    // --- Safety timeout ---
    if (now - startTime > AUTOTUNE_TIMEOUT_MS) {
        Serial.println("[AutoTune] Timeout — sem oscilacao suficiente");
        state = AUTOTUNE_FAILED;
        relayOutput = 0.0;
        return;
    }

    // --- Relay control: high while below setpoint, low at/above it ---
    float error = setpoint - measurement;
    relayOutput = (error > 0.0) ? relayHigh : relayLow;

    // --- Track oscillation envelope ---
    // Só a partir do 2º cruzamento: o primeiro semiciclo (partida) não
    // faz parte do ciclo limite. O envelope é reiniciado no cruzamento 2.
    if (crossCount >= 2) {
        if (measurement < rpmMin) rpmMin = measurement;
        if (measurement > rpmMax) rpmMax = measurement;
    }

    // --- Detect setpoint crossing (error sign flip) ---
    if ((prevError > 0.0 && error <= 0.0) || (prevError < 0.0 && error >= 0.0)) {
        // Debounce (retriggerable): flips mais rápidos que o plausível
        // para o motor são picos residuais de medição e não contam.
        if (now - lastCrossTime >= AUTOTUNE_MIN_CROSS_MS) {
            crossCount++;

            // Reinicia o envelope no 2º cruzamento: dali em diante a
            // oscilação é o ciclo limite que queremos medir.
            if (crossCount == 2) {
                rpmMin = measurement;
                rpmMax = measurement;
            }

            // Um período completo = intervalo entre cruzamentos PARES
            // consecutivos (ex.: cruzamento 4 - cruzamento 2).
            if (crossCount % 2 == 0) {
                if (crossCount > 2) {
                    unsigned long period = now - lastPeriodAnchor;
                    periodSum += period;
                    periodCount++;
                }
                lastPeriodAnchor = now;
            }
            Serial.printf("[AutoTune] cruzamento %d: RPM=%.0f (%.0f ms)\n",
                          crossCount, measurement, (float)(now - lastAcceptedCrossTime));
            lastAcceptedCrossTime = now;
        }
        lastCrossTime = now;
    }
    prevError = error;

    // --- Finish when enough periods collected ---
    if (periodCount >= requiredPeriods && periodCount > 0) {
        float tu = (float)periodSum / (float)periodCount / 1000.0; // seconds
        float a = (rpmMax - rpmMin) / 2.0; // oscillation amplitude (RPM)

        if (a > 0.01) {
            resultTu = tu;
            // Ku = 4*d / (pi*a), com d = amplitude efetiva do relay (% PWM)
            resultKu = (4.0 * relayStep) / (PI * a);

            state = AUTOTUNE_DONE;
            Serial.print("[AutoTune] Concluido: Ku=");
            Serial.print(resultKu);
            Serial.print(" Tu=");
            Serial.println(resultTu);
        } else {
            Serial.println("[AutoTune] Falha — amplitude de oscilacao nula");
            state = AUTOTUNE_FAILED;
        }
        relayOutput = 0.0;
    }
}

float autotuneGetRelayOutput() {
    return relayOutput;
}

bool autotuneIsRunning() {
    return state == AUTOTUNE_RUNNING;
}

bool autotuneIsDone() {
    return state == AUTOTUNE_DONE;
}

bool autotuneIsFailed() {
    return state == AUTOTUNE_FAILED;
}

float autotuneGetKu() {
    return resultKu;
}

float autotuneGetTu() {
    return resultTu;
}

int autotuneGetProgress() {
    if (state == AUTOTUNE_DONE) return 100;
    if (state != AUTOTUNE_RUNNING) return 0;
    // Progress estimated by collected periods (each needs 2 crossings)
    int target = requiredPeriods * 2 + 1;
    int done = crossCount;
    if (done > target) done = target;
    return (int)((100L * done) / target);
}
