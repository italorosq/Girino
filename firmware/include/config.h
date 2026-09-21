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

// --- Wi-Fi (Access Point) ---
// O Girino opera como Access Point: cria sua própria rede para que
// os alunos conectem direto pelo navegador, sem roteador nem internet.
// Credenciais sobrescritáveis via build_flags (.env).
// IMPORTANTE: a senha do AP precisa ter no mínimo 8 caracteres.
#ifndef WIFI_SSID
#define WIFI_SSID "GIRINO_AP"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "girino123"
#endif

#ifndef OTA_PASSWORD
#define OTA_PASSWORD "girino"
#endif

// IP fixo do Girino na rede do AP (gateway para os clientes conectados)
#define AP_LOCAL_IP IPAddress(192, 168, 4, 1)

// --- Motor ---
// PWM por software do ESP8266: o core limita frequência x passos a ~1 MHz.
// 4 kHz x 255 passos (8-bit) deixa o chiado do chaveamento mais discreto
// e mantém 0,4% de resolução de duty.
// Bancada medida: zona morta ~60%, 100% -> ~1950 RPM (10 V no motor).
#define MOTOR_PWM_FREQ     4000    // Frequência PWM em Hz
#define MOTOR_PWM_RANGE    255     // Resolução PWM (8-bit)
#define MOTOR_MAX_SPEED    100     // Velocidade máxima em %

// --- Encoder LPD3806-600BM ---
#define ENCODER_PPR        600     // Pulsos por revolução
#define ENCODER_SAMPLE_MS  100     // Intervalo de amostragem para cálculo de RPM

// --- Feedback do encoder (didático) ---
// O LED onboard pisca a cada pulso recebido — dá para conferir a
// fiação/leitura do encoder sem abrir o navegador.
#define ENCODER_LED_ACTIVITY_MS   60     // Tempo que o LED fica aceso (ms)
#define ENCODER_DEBUG_INTERVAL_MS 1000   // Telemetria no serial (ms); 0 desativa

// --- Direções do motor ---
#define MOTOR_DIR_FORWARD   1
#define MOTOR_DIR_REVERSE  -1
#define MOTOR_DIR_STOP      0

// --- PID / Auto-Tune ---
#define PID_SAMPLE_MS       20      // Período de amostragem do PID (50 Hz)
#define PID_OUTPUT_MAX      100.0   // Saída máxima do PID (% PWM)
#define PID_OUTPUT_MIN      0.0     // Saída mínima do PID (% PWM)
#define PID_MAX_SETPOINT    3000.0  // Limite de setpoint de velocidade (RPM)
#define PID_MAX_PPR_WINDOW  20.0    // Janela de leitura do encoder (ms) p/ RPM do PID

// --- Aquisição de velocidade no laço de controle ---
// O Wi-Fi do ESP8266 bloqueia interrupções em rajadas, então contagens
// de 20 ms podem perder pulsos e "pular". A janela deslizante e o
// limitador de slew rejeitam essas leituras impossíveis.
#define CONTROL_RPM_WINDOW_TICKS 4      // janela deslizante (4 x 20 ms = 80 ms)
#define CONTROL_RPM_SLEW_MAX     8000.0 // aceleração máxima plausível (RPM/s)

// --- Controle de posição (ângulo) ---
// O PID de posição é bidirecional: saída positiva gira horário,
// saída negativa gira anti-horário. A zona morta evita trepidação
// do motor quando o erro é pequeno (ruído do encoder).
#define POS_OUTPUT_MAX      100.0   // Saída máxima (% PWM) em módulo
#define POS_OUTPUT_MIN     -100.0   // Saída mínima (-100% = reverso)
#define POS_DEADBAND_DEG    2.0     // Zona morta em graus
#define POS_MAX_TARGET_DEG  3600.0  // Limite de alvo (10 voltas)

// --- Auto-Tune (relay feedback) ---
// O relay alterna entre (bias + amplitude) e (bias - amplitude). O bias
// (pedestal) precisa ficar ACIMA da zona morta do motor — um motor DC só
// vence o atrito estático com um PWM mínimo. Medido nesta bancada: o eixo
// começa a girar bem com ~60% de duty (chiado abaixo/disso é o
// chaveamento do PWM, normal). Faixa padrão do relay: 64% a 100%.
#define AUTOTUNE_DEFAULT_RELAY  18  // Amplitude padrão do relay (% PWM)
#define AUTOTUNE_DEFAULT_BIAS   82  // Pedestal padrão (% PWM) — acima da zona morta
#define AUTOTUNE_BIAS_MAX       95  // Limite do pedestal (% PWM)
#define AUTOTUNE_DEFAULT_CYCLES 3   // Ciclos padrão de oscilação
#define AUTOTUNE_DEFAULT_SETPOINT 1000.0 // Setpoint padrão do experimento (RPM)
#define AUTOTUNE_MIN_CROSS_MS   60  // Debounce de cruzamentos (ms) — ignora ruído
#define AUTOTUNE_TIMEOUT_MS     30000 // Timeout de segurança (30 s)

#endif // CONFIG_H
