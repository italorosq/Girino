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

// --- Pulso de calibração do sentido da fiação ---
// Pulso curto no primeiro movimento após o boot para descobrir o sentido
// físico motor/encoder. Curva real medida na bancada: zona morta ~65%,
// joelho íngreme (70% -> ~400 RPM) — não dá para calibrar devagar sem o
// atrito estático travar. Estratégia: pulso de 70% por 50 ms (giro
// ~45-60°, ~80+ pulsos — limiar: 30) SEGUIDO DE FREIO, que mata a
// inércia (antes, roda-livre + 100%/150 ms girava 400°+ = mais de uma
// volta, parecia overshoot no gráfico logo ao iniciar).
#define MOTOR_CALIB_DUTY     70
#define MOTOR_CALIB_MS       50
#define MOTOR_CALIB_BRAKE_MS 40

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
#define MOTOR_DIR_STOP      0    // roda-livre (L298N: IN3=IN4=LOW, EN=0)
#define MOTOR_DIR_BRAKE     2    // freio dinâmico (L298N: IN3=IN4=HIGH)

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
// Zona morta do MOTOR em duty (medida na bancada ~60%): abaixo disso o
// PWM não vence o atrito estático. Na malha de posição, uma saída nessa
// faixa não consegue mover nada — em vez de roda-livre, o firmware
// FREIA o eixo (mata a inércia que causava o overshoot de cada chute).
#define POS_MOTOR_DEADZONE_DUTY 60.0
// Suavização perto do alvo: o motor real escapa rápido demais e chutes a
// 100% passam do alvo por dezenas de graus (caça ±35° medida). Com erro
// pequeno, o duty é limitado a pouco acima da zona morta — cada chute
// vira um passo curto e a caça assenta dentro da deadband (±2°).
#define POS_SOFT_NEAR_DEG   30.0  // erro abaixo disso ativa o modo passo
#define POS_SOFT_DUTY       65.0  // duty dos passos perto do alvo (% PWM)
// Passos discretos perto do alvo (a 50 Hz): KICK ticks de chute + REST
// de freio — cada passo move poucos graus e o eixo assenta na deadband.
// Rest longo (160 ms): o L298N sofre com ciclos freio/acionamento muito
// rápidos (desligamento térmico medido na bancada) — o descanso esfria
// o driver e elimina o zumbido.
#define POS_SOFT_KICK_TICKS 1
#define POS_SOFT_REST_TICKS 8
// Duty da malha de posição. Medido na bancada (curva real): abaixo de
// ~66% o motor trava no atrito estático (movimento errático), 70% já
// responde com ~400 RPM. O duty fica na região CONFIÁVEL.
#define POS_MAX_DUTY        70.0
// Zona de frenagem antecipada: chegando perto do alvo (erro abaixo de
// 60°) e ainda em movimento (> 100 RPM), FREIA antes de entrar na zona
// de passos — a velocidade de entrada é o que gerava o overshoot.
#define POS_BRAKE_ZONE_DEG  60.0
#define POS_BRAKE_ZONE_RPM  100.0
// O PID "pede movimento" a partir desta saída; abaixo disso (ganhos
// zerados ou erro ~0) o eixo fica freado.
#define POS_DRIVE_EPS       1.0
// Teto do TERMO INTEGRAL da posição (em % de saída): mantém a parcela
// integral abaixo da zona morta do motor (60%). Sem isso, com erro ~0 o
// termo integral sozinho sustenta a saída acima de 60% e o motor segue
// acionado, rastejando além do alvo. O Kp cuida da aproximação; o Ki
// só precisa corrigir desvios pequenos.
#define POS_INTEGRAL_TERM_MAX 55.0
// Teto do TERMO DERIVATIVO da posição: com o eixo a toda velocidade o
// de/dt passa de 10.000 °/s e o D sozinho inverteria a saída a cada tick
// (slam de ±100%) — a causa principal da caça ±35° na bancada. Clampear
// a contribuição do D mantém o amortecimento sem o soco.
#define POS_DERIVATIVE_TERM_MAX 30.0

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
// Timeout do autotune de POSIÇÃO: os ciclos da malha de ângulo são muito
// mais lentos que os de velocidade (vários segundos por oscilação),
// então 3 ciclos não cabem nos 30 s do autotune de velocidade.
#define AUTOTUNE_TIMEOUT_MS_POS 90000 // Timeout de segurança posição (90 s)
// Amplitude padrão do relay de posição (± d, simétrico). Fica bem acima
// da zona morta (~60%), garantindo movimento nos dois sentidos sem
// bater a 100% de duty.
#define AUTOTUNE_POS_DEFAULT_RELAY 90 // ± % PWM
#define AUTOTUNE_POS_MIN_RELAY     40 // abaixo de ~60% (zona morta) não oscila

#endif // CONFIG_H
