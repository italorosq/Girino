/**
 * encoder.cpp — Implementação da leitura do encoder LPD3806-600BM
 *
 * Usa interrupção GPIO no canal A para contagem de pulsos.
 * O canal B é lido dentro da ISR para determinar a direção.
 */

#include <Arduino.h>
#include "encoder.h"
#include "pins.h"
#include "config.h"

// --- Variáveis voláteis (acessadas na ISR) ---
static volatile long pulseCount = 0;
static volatile long lastPulseCount = 0;

// --- Variáveis de cálculo de RPM ---
static unsigned long lastSampleTime = 0;
static float currentRPM = 0.0;

/**
 * ISR: chamada a cada transição do canal A do encoder.
 * Lê o canal B para determinar a direção de rotação.
 */
void ICACHE_RAM_ATTR encoderISR() {
    int bState = digitalRead(ENCODER_B_PIN);
    if (bState == HIGH) {
        pulseCount++;   // Sentido horário
    } else {
        pulseCount--;   // Sentido anti-horário
    }
}

void encoderInit() {
    pinMode(ENCODER_A_PIN, INPUT_PULLUP);
    pinMode(ENCODER_B_PIN, INPUT_PULLUP);

    // Interrupção no canal A — borda de subida
    attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, RISING);

    lastSampleTime = millis();
    Serial.println("[Encoder] Inicializado — LPD3806-600BM (600 PPR)");
}

void encoderUpdate() {
    unsigned long now = millis();
    unsigned long elapsed = now - lastSampleTime;

    if (elapsed >= ENCODER_SAMPLE_MS) {
        // Calcular RPM baseado na variação de pulsos
        noInterrupts();
        long currentPulses = pulseCount;
        long deltaPulses = currentPulses - lastPulseCount;
        lastPulseCount = currentPulses;
        interrupts();

        // RPM = (pulsos / PPR) * (60000 / elapsed_ms)
        // PPR = 600 pulsos por revolução
        currentRPM = ((float)abs(deltaPulses) / (float)ENCODER_PPR) * (60000.0 / (float)elapsed);

        lastSampleTime = now;
    }
}

long encoderGetPulses() {
    noInterrupts();
    long p = pulseCount;
    interrupts();
    return p;
}

float encoderGetRPM() {
    return currentRPM;
}

void encoderReset() {
    noInterrupts();
    pulseCount = 0;
    lastPulseCount = 0;
    currentRPM = 0.0;
    interrupts();
    Serial.println("[Encoder] Contagem resetada");
}
