/**
 * pins.h — Definição centralizada dos pinos GPIO do ESP8266
 *
 * Mapeamento de pinos para o hardware Girino.
 * Referência: ESP8266 WROOM (NodeMCU / genérico)
 *
 * GPIO -> Função
 * ──────────────────────────────────
 * D1 (GPIO5)  -> Encoder canal A
 * D2 (GPIO4)  -> Encoder canal B
 * D5 (GPIO14) -> Motor PWM
 * D6 (GPIO12) -> Motor direção (IN1)
 * D7 (GPIO13) -> Motor direção (IN2)
 */

#ifndef PINS_H
#define PINS_H

// --- Encoder LPD3806-600BM ---
#define ENCODER_A_PIN    5    // GPIO5  (D1)
#define ENCODER_B_PIN    4    // GPIO4  (D2)

// --- Controle do Motor DC ---
#define MOTOR_PWM_PIN    14   // GPIO14 (D5)
#define MOTOR_DIR_PIN1   12   // GPIO12 (D6)
#define MOTOR_DIR_PIN2   13   // GPIO13 (D7)

// --- LED onboard (debug) ---
#define LED_BUILTIN_PIN  2    // GPIO2 (LED onboard ativo-baixo)

#endif // PINS_H
