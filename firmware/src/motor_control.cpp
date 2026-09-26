/**
 * motor_control.cpp — Implementação do controle PWM de motor DC
 */

#include <Arduino.h>
#include "motor_control.h"
#include "encoder.h"
#include "pins.h"
#include "config.h"

static int currentSpeed = 0;
static int currentDirection = MOTOR_DIR_STOP;

// --- Auto-calibração do sentido da fiação ---
// No primeiro movimento após o boot, um pulso de teste revela o sentido
// físico. Se a fiação estiver invertida, o mapa de direção é corrigido
// em software — não importa como o motor/encoder foram ligados.
static bool dirInverted = false;
static bool calibrationDone = false;
static bool calibrating = false;

void motorControlInit() {
    pinMode(MOTOR_PWM_PIN, OUTPUT);
    pinMode(MOTOR_DIR_PIN1, OUTPUT);
    pinMode(MOTOR_DIR_PIN2, OUTPUT);

    analogWriteFreq(MOTOR_PWM_FREQ);
    analogWriteRange(MOTOR_PWM_RANGE);

    motorSetSpeed(0);
    motorSetDirection(MOTOR_DIR_STOP);

    Serial.println("[Motor] Inicializado");
}

void motorSetSpeed(int speed) {
    // Primeiro movimento após o boot: calibra o sentido da fiação
    // (pulso curto de teste — roda uma única vez por boot).
    if (speed > 0 && !calibrationDone && !calibrating) {
        motorCalibrateDirection();
    }

    // Limitar ao range permitido
    if (speed < 0) speed = 0;
    if (speed > MOTOR_MAX_SPEED) speed = MOTOR_MAX_SPEED;

    currentSpeed = speed;

    // Converter percentual para valor PWM
    int pwmValue = map(speed, 0, MOTOR_MAX_SPEED, 0, MOTOR_PWM_RANGE);
    analogWrite(MOTOR_PWM_PIN, pwmValue);
}

void motorSetDirection(int direction) {
    currentDirection = direction;

    // Aplica a inversão detectada: o mapa lógico continua o mesmo
    // (FORWARD = horário), mas as saídas físicas são trocadas.
    int d = direction;
    if (dirInverted) {
        if (d == MOTOR_DIR_FORWARD) d = MOTOR_DIR_REVERSE;
        else if (d == MOTOR_DIR_REVERSE) d = MOTOR_DIR_FORWARD;
    }

    switch (d) {
        case MOTOR_DIR_FORWARD:
            digitalWrite(MOTOR_DIR_PIN1, HIGH);
            digitalWrite(MOTOR_DIR_PIN2, LOW);
            break;

        case MOTOR_DIR_REVERSE:
            digitalWrite(MOTOR_DIR_PIN1, LOW);
            digitalWrite(MOTOR_DIR_PIN2, HIGH);
            break;

        case MOTOR_DIR_BRAKE:
            // L298N: EN em nível alto + ambas as entradas HIGH curto-circui-
            // tam as fases do motor — freio dinâmico. O PWM precisa ficar
            // em nível alto para habilitar o driver (EN=0 seria roda-livre).
            digitalWrite(MOTOR_DIR_PIN1, HIGH);
            digitalWrite(MOTOR_DIR_PIN2, HIGH);
            analogWrite(MOTOR_PWM_PIN, MOTOR_PWM_RANGE);
            break;

        case MOTOR_DIR_STOP:
        default:
            digitalWrite(MOTOR_DIR_PIN1, LOW);
            digitalWrite(MOTOR_DIR_PIN2, LOW);
            break;
    }
}

/**
 * Freio dinâmico: trava o eixo no lugar (ambas as fases em HIGH).
 * Usado pela malha de posição quando a saída está abaixo da zona morta
 * do motor — o freio mata a inércia que causava o overshoot de cada
 * "chute" do bang-bang, e segura o eixo firme parado.
 */
void motorBrake() {
    currentDirection = MOTOR_DIR_BRAKE;
    currentSpeed = 0;            // sem acionamento; o PWM alto é só o EN do freio
    motorSetDirection(MOTOR_DIR_BRAKE);
}

bool motorDirInverted() {
    return dirInverted;
}

void motorCalibrateDirection() {
    if (calibrationDone || calibrating) return;
    calibrating = true;

    Serial.println("[Motor] Calibrando sentido da fiacao (pulso de teste)...");

    long pulsesBefore = encoderGetPulses();

    // Pulso curto e SUAVE no sentido forward ATUAL (mapa ainda padrão).
    // Duty deliberadamente moderado (ver MOTOR_CALIB_DUTY): o objetivo é
    // só ler a direção dos pulsos — o eixo deve girar poucos graus, não
    // disparar a 1950 RPM (aquele pulso parecia overshoot no gráfico).
    digitalWrite(MOTOR_DIR_PIN1, HIGH);
    digitalWrite(MOTOR_DIR_PIN2, LOW);
    analogWrite(MOTOR_PWM_PIN, map(MOTOR_CALIB_DUTY, 0, MOTOR_MAX_SPEED, 0, MOTOR_PWM_RANGE));
    delay(MOTOR_CALIB_MS);
    // FREIA imediatamente: sem isso o eixo continua de inércia depois do
    // pulso e o giro total fica várias vezes maior que o pulso.
    digitalWrite(MOTOR_DIR_PIN1, HIGH);
    digitalWrite(MOTOR_DIR_PIN2, HIGH);
    analogWrite(MOTOR_PWM_PIN, MOTOR_PWM_RANGE);
    delay(MOTOR_CALIB_BRAKE_MS);
    analogWrite(MOTOR_PWM_PIN, 0);
    digitalWrite(MOTOR_DIR_PIN1, LOW);
    digitalWrite(MOTOR_DIR_PIN2, LOW);
    delay(50);

    long delta = encoderGetPulses() - pulsesBefore;

    if (delta < -30) {
        dirInverted = true;
        Serial.println("[Motor] Fiacao invertida detectada — corrigido em software");
    } else if (delta > 30) {
        dirInverted = false;
        Serial.print("[Motor] Sentido OK (");
        Serial.print(delta);
        Serial.println(" pulsos no teste)");
    } else {
        Serial.println("[Motor] Calibracao inconclusiva (motor nao girou?) — mantendo padrao");
    }

    calibrationDone = true;
    calibrating = false;

    // Restaura a direção lógica atual com o mapa já corrigido
    motorSetDirection(currentDirection);
}

int motorGetSpeed() {
    return currentSpeed;
}

int motorGetDirection() {
    return currentDirection;
}
