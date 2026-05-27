/**
 * config.h — Constantes e configurações do firmware Girino
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "pins.h"

// --- Versão do firmware ---
#define FIRMWARE_VERSION "0.1.0"

// --- Serial ---
#ifndef BAUD_RATE
#define BAUD_RATE 115200
#endif

// --- WiFi ---
// Credenciais injetadas via build_flags (.env)
#ifndef WIFI_SSID
#define WIFI_SSID "GIRINO_AP"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "girino123"
#endif

#ifndef OTA_PASSWORD
#define OTA_PASSWORD "girino"
#endif

#define WIFI_TIMEOUT_MS 15000  // 15 segundos para conectar

// --- Motor ---
#define MOTOR_PWM_FREQ     1000    // Frequência PWM em Hz
#define MOTOR_PWM_RANGE    1023    // Resolução PWM (10-bit)
#define MOTOR_MAX_SPEED    100     // Velocidade máxima em %

// --- Encoder LPD3806-600BM ---
#define ENCODER_PPR        600     // Pulsos por revolução
#define ENCODER_SAMPLE_MS  100     // Intervalo de amostragem para cálculo de RPM

// --- Direções do motor ---
#define MOTOR_DIR_FORWARD   1
#define MOTOR_DIR_REVERSE  -1
#define MOTOR_DIR_STOP      0

#endif // CONFIG_H
