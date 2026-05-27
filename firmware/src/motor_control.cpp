/**
 * motor_control.cpp — Implementação do controle PWM de motor DC
 */

#include <Arduino.h>
#include "motor_control.h"
#include "pins.h"
#include "config.h"

static int currentSpeed = 0;
static int currentDirection = MOTOR_DIR_STOP;

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

    switch (direction) {
        case MOTOR_DIR_FORWARD:
            digitalWrite(MOTOR_DIR_PIN1, HIGH);
            digitalWrite(MOTOR_DIR_PIN2, LOW);
            break;

        case MOTOR_DIR_REVERSE:
            digitalWrite(MOTOR_DIR_PIN1, LOW);
            digitalWrite(MOTOR_DIR_PIN2, HIGH);
            break;

        case MOTOR_DIR_STOP:
        default:
            digitalWrite(MOTOR_DIR_PIN1, LOW);
            digitalWrite(MOTOR_DIR_PIN2, LOW);
            break;
    }
}

int motorGetSpeed() {
    return currentSpeed;
}

int motorGetDirection() {
    return currentDirection;
}
