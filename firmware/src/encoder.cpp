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
 *
 * Convenção: o sentido HORÁRIO (motor em FORWARD) conta positivo — o
 * ângulo cresce no sentido "horário" da interface e o controle de
 * posição converge com alvos positivos.
 *
 * NOTA: a polaridade depende da fiação (motor e canais A/B). Se o modo
 * Posição divergir, verifique a fiação ou inverta as ramificações abaixo;
 * as malhas de velocidade/auto-tune são imunes (usam |RPM|).
 */
void IRAM_ATTR encoderISR() {
    int bState = digitalRead(ENCODER_B_PIN);
    if (bState == HIGH) {
        pulseCount++;   // Horário (FORWARD do motor)
    } else {
        pulseCount--;   // Anti-horário
    }
}

void encoderInit() {
    // O LPD3806 tem saída em COLETOR ABERTO: cada canal só puxa o pino para
    // GND. Sem resistor de pull-up o sinal fica flutuando e não é legível.
    // A leitura depende dos RESISTORES EXTERNOS de 4.7k soldados na placa,
    // puxando os canais A e B para 3.3V (NUNCA para 5V — os GPIOs do
    // ESP8266 não toleram 5V).
    //
    // Por isso usamos INPUT (sem pull-up interno): o pull-up do ESP8266 é
    // fraco e impreciso; o resistor externo dá bordas firmes e contagem
    // confiável em RPM alto.
    pinMode(ENCODER_A_PIN, INPUT);
    pinMode(ENCODER_B_PIN, INPUT);

    // Interrupção no canal A — borda de subida
    attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, RISING);

    // LED onboard para feedback visual dos pulsos (ativo-baixo)
    pinMode(LED_BUILTIN_PIN, OUTPUT);
    digitalWrite(LED_BUILTIN_PIN, HIGH);  // apagado

    lastSampleTime = millis();
    Serial.println("[Encoder] Inicializado — LPD3806-600BM (600 PPR)");
}

void encoderUpdateFeedback() {
    unsigned long now = millis();
    long pulses = encoderGetPulses();

    // --- LED onboard: acende por ENCODER_LED_ACTIVITY_MS após cada pulso ---
    // Girar o eixo devagar já pisca o LED; sem sinal, LED fica apagado.
    static long lastLedPulses = 0;
    static unsigned long ledUntil = 0;
    if (pulses != lastLedPulses) {
        lastLedPulses = pulses;
        ledUntil = now + ENCODER_LED_ACTIVITY_MS;
    }
    bool ledOn = (long)(now - ledUntil) < 0;  // ainda dentro da janela?
    digitalWrite(LED_BUILTIN_PIN, ledOn ? LOW : HIGH);

#if ENCODER_DEBUG_INTERVAL_MS > 0
    // --- Telemetria no serial: prova de sinal mesmo sem motor ---
    static unsigned long lastPrint = 0;
    static long lastPrintPulses = 0;
    if (now - lastPrint >= ENCODER_DEBUG_INTERVAL_MS) {
        float dt = (now - lastPrint) / 1000.0;
        float rate = (lastPrint > 0) ? (pulses - lastPrintPulses) / dt : 0.0;
        Serial.printf("[Encoder] pulsos=%ld (%.1f p/s) rpm=%.1f angulo=%.1f\n",
                      pulses, rate, encoderGetRPM(), encoderGetAngle());
        lastPrintPulses = pulses;
        lastPrint = now;
    }
#endif
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

float encoderGetAngle() {
    // 600 pulsos = 1 revolução = 360°  =>  ângulo = pulsos * 360 / PPR
    return ((float)encoderGetPulses() / (float)ENCODER_PPR) * 360.0;
}

void encoderReset() {
    noInterrupts();
    pulseCount = 0;
    lastPulseCount = 0;
    currentRPM = 0.0;
    interrupts();
    Serial.println("[Encoder] Contagem resetada");
}
