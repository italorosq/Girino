/**
 * Girino — Plataforma didática de controle de motor DC
 * LaRA — Laboratório de Robótica e Automação
 *
 * Firmware principal para ESP8266 WROOM.
 * Integra controle PWM de motor, leitura de encoder incremental,
 * controle PID em malha fechada, auto-tune via relay feedback
 * e atualização OTA via Wi-Fi.
 *
 * Modos de operação (para comparação didática):
 *   - Malha aberta: PWM manual via /api/motor
 *   - Malha fechada: PID via /api/pid/...
 *   - Auto-tune: relay feedback via /api/pid/autotune
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ElegantOTA.h>
#include <LittleFS.h>

#include "pins.h"
#include "config.h"
#include "motor_control.h"
#include "encoder.h"
#include "pid.h"
#include "pid_autotune.h"
#include "pid_tuning_rules.h"
#include "serial_console.h"

// --- Web Server na porta 80 ---
ESP8266WebServer server(80);

// --- Protótipos ---
void setupWiFi();
void setupWebServer();
void handleMotorCommand();
void handleEncoderRead();
void handleStatus();
void handlePidConfig();
void handlePidStart();
void handlePidStop();
void handleAutotune();
void handlePidTuning();
void handlePidTuningApply();
void handlePidResponse();
void handlePositionConfig();
void handlePositionStart();
void handlePositionStop();
void handlePositionZero();
void controlTick(float rpm, float dt);
void recordResponse(unsigned long tMs, float rpm);
void resetResponseBuffer();
String gainsToJson(const PidGains& g);
String posConfigToJson();

// =============================================================
// BUFFER DE RESPOSTA AO DEGRAU
// =============================================================
// Amostras (t, RPM) registradas a cada tick de controle enquanto
// o PID ou o auto-tune estão ativos. Servidas via /api/pid/response.
#define RESPONSE_MAX_POINTS 512
struct ResponsePoint {
    uint16_t tMs;   // offset desde o início do experimento (ms)
    float value;    // medida do experimento (RPM ou graus, conforme unidade)
};
static ResponsePoint responseBuffer[RESPONSE_MAX_POINTS];
static int responseLen = 0;
static unsigned long responseStart = 0;
// Unidade da resposta ao degrau: "rpm" (velocidade) ou "deg" (posição)
static const char* responseUnit = "rpm";

// =============================================================
// SETUP
// =============================================================
void setup() {
    Serial.begin(BAUD_RATE);
    Serial.println();
    Serial.print("=== Girino v");
    Serial.print(FIRMWARE_VERSION);
    Serial.println(" ===");
    Serial.println("LaRA — Laboratório de Robótica e Automação");

    // Inicializar módulos
    motorControlInit();
    encoderInit();
    pidInit();
    autotuneInit();

    // Conectar Wi-Fi
    setupWiFi();

    // Inicializar filesystem
    if (!LittleFS.begin()) {
        Serial.println("[LittleFS] Falha ao montar filesystem");
    } else {
        Serial.println("[LittleFS] Filesystem montado");
    }

    // Configurar servidor web + OTA
    setupWebServer();

    // Console de comandos via serial (USB)
    serialConsoleInit();

    Serial.println("Sistema inicializado com sucesso!");
    Serial.print("Pagina de controle: http://");
    Serial.println(WiFi.softAPIP().toString());
    Serial.print("OTA: http://");
    Serial.println(WiFi.softAPIP().toString() + "/update");
}

// =============================================================
// LOOP
// =============================================================
static unsigned long lastControlTick = 0;
static long lastControlPulses = 0;

void loop() {
    server.handleClient();
    ElegantOTA.loop();
    encoderUpdate();
    encoderUpdateFeedback();  // LED + telemetria do encoder (didático)
    serialConsoleUpdate();    // comandos via serial (USB)

    // Tick de controle em tempo real (50 Hz) — PID e auto-tune
    unsigned long now = millis();
    if (now - lastControlTick >= PID_SAMPLE_MS) {
        float dt = (now - lastControlTick) / 1000.0;
        lastControlTick = now;

        long pulses = encoderGetPulses();
        long delta = pulses - lastControlPulses;
        lastControlPulses = pulses;

        // Janela deslizante (4 ticks = 80 ms): suaviza pulsos perdidos
        // nas rajadas de Wi-Fi, que gerariam picos impossíveis em 20 ms.
        static long deltaRing[CONTROL_RPM_WINDOW_TICKS] = {0};
        static float dtRing[CONTROL_RPM_WINDOW_TICKS] = {0};
        static uint8_t ringIdx = 0;
        deltaRing[ringIdx] = delta;
        dtRing[ringIdx] = dt;
        ringIdx = (ringIdx + 1) % CONTROL_RPM_WINDOW_TICKS;

        long deltaSum = 0;
        float dtSum = 0.0;
        for (int i = 0; i < CONTROL_RPM_WINDOW_TICKS; i++) {
            deltaSum += deltaRing[i];
            dtSum += dtRing[i];
        }
        float rpmRaw = (dtSum > 0.0)
                           ? ((float)deltaSum / (float)ENCODER_PPR) * (60.0 / dtSum)
                           : 0.0;

        // Limitador de slew: o motor não varia mais que CONTROL_RPM_SLEW_MAX
        // por segundo — saltos maiores são ruído de medição e são aparados.
        static float rpmCtrl = 0.0;
        float maxStep = CONTROL_RPM_SLEW_MAX * dt;
        if (rpmRaw > rpmCtrl + maxStep) rpmRaw = rpmCtrl + maxStep;
        if (rpmRaw < rpmCtrl - maxStep) rpmRaw = rpmCtrl - maxStep;
        rpmCtrl = rpmRaw;

        controlTick(rpmCtrl, dt);
    }
}

/**
 * Tick de controle: decide quem aciona o motor.
 * Prioridade: auto-tune > posição > velocidade (PID) > malha aberta.
 */
void controlTick(float rpm, float dt) {
    // Telemetria didática (2x/s) enquanto um controlador está ativo —
    // permite acompanhar o experimento pelo monitor serial.
    static unsigned long lastTelemetry = 0;
    unsigned long nowMs = millis();
    bool verbose = (nowMs - lastTelemetry >= 500);
    if (verbose) lastTelemetry = nowMs;

    if (autotuneIsRunning()) {
        // Relay é unidirecional (só horário): usa o módulo da velocidade
        // para ficar imune à inversão de fiação motor/encoder.
        float rpmAbs = fabs(rpm);
        autotuneCompute(rpmAbs, dt);
        float out = autotuneGetRelayOutput();
        motorSetDirection(MOTOR_DIR_FORWARD);
        motorSetSpeed((int)out);
        recordResponse(millis() - responseStart, rpmAbs);
        if (verbose) Serial.printf("[AutoTune] RPM=%.0f relay=%.0f%%\n", rpmAbs, out);

        // Experimento terminou?
        if (autotuneIsDone() || autotuneIsFailed()) {
            motorSetSpeed(0);
            motorSetDirection(MOTOR_DIR_STOP);
        }
    } else if (posIsRunning()) {
        // Malha de posição: saída com sinal define o sentido de giro
        float angle = encoderGetAngle();
        float out = posCompute(angle, dt);
        if (out >= 0.0) {
            motorSetDirection(MOTOR_DIR_FORWARD);
            motorSetSpeed((int)out);
        } else {
            motorSetDirection(MOTOR_DIR_REVERSE);
            motorSetSpeed((int)(-out));
        }
        recordResponse(millis() - responseStart, angle);
        if (verbose) Serial.printf("[Pos] Alvo=%.1f Angulo=%.1f Out=%.0f%%\n", posGetTarget(), angle, out);
    } else if (pidIsRunning()) {
        // A malha de velocidade é unidirecional (só horário): usa o módulo
        // da velocidade como realimentação — assim continua regulando mesmo
        // se a fiação do motor/encoder estiver invertida.
        float rpmAbs = fabs(rpm);
        float out = pidCompute(rpmAbs, dt);
        motorSetDirection(MOTOR_DIR_FORWARD);
        motorSetSpeed((int)out);
        recordResponse(millis() - responseStart, rpmAbs);
        if (verbose) Serial.printf("[PID] SP=%.0f RPM=%.0f Out=%.0f%%\n", pidGetSetpoint(), rpmAbs, out);

        // Aviso didático: giro no sentido oposto ao esperado pela fiação.
        static bool polarityWarned = false;
        if (rpm < -50.0 && !polarityWarned) {
            polarityWarned = true;
            Serial.println("[PID] Aviso: motor gira ao contrario da fiacao esperada — verifique motor/encoder");
        }
    }
    // Sem PID nem auto-tune: motor segue o último comando manual
    // (tratado em handleMotorCommand) — malha aberta para comparação.
}

// =============================================================
// BUFFER DE RESPOSTA
// =============================================================
void resetResponseBuffer() {
    responseLen = 0;
    responseStart = millis();
}

/**
 * Define a unidade da resposta ao degrau ("rpm" ou "deg").
 * Usado pelos controladores (web e console serial).
 */
void setResponseUnit(const char* unit) {
    responseUnit = unit;
}

void recordResponse(unsigned long tMs, float value) {
    if (responseLen >= RESPONSE_MAX_POINTS) return;
    responseBuffer[responseLen].tMs = (uint16_t)tMs;
    responseBuffer[responseLen].value = value;
    responseLen++;
}

// =============================================================
// Implementações — Wi-Fi e servidor
// =============================================================

void setupWiFi() {
    // Modo Access Point: o Girino cria sua própria rede Wi-Fi.
    // Vantagem didática: alunos conectam direto pelo celular ou
    // notebook, sem depender de roteador, internet ou credenciais
    // de rede externa. O IP fixo 192.168.4.1 é o endereço da página.
    WiFi.mode(WIFI_AP);

    // IMPORTANTE: configurar o IP ANTES de subir o AP. Se chamado
    // depois, o servidor DHCP embutido não reinicia e os clientes
    // conectam sem receber endereço IP.
    IPAddress ip = AP_LOCAL_IP;
    WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));

    // Senha do AP precisa ter >= 8 caracteres; caso contrário,
    // sobe uma rede aberta (sem senha) para não bloquear a aula.
    const char* apPass = WIFI_PASSWORD;
    bool apOpen = (strlen(apPass) < 8);
    bool ok;
    if (apOpen) {
        ok = WiFi.softAP(WIFI_SSID);
        Serial.println("[WiFi] Senha do AP curta (<8 chars) — rede aberta!");
    } else {
        ok = WiFi.softAP(WIFI_SSID, apPass);
    }

    if (ok) {
        Serial.printf("[WiFi] Access Point '%s' ativo\n", WIFI_SSID);
        Serial.printf("[WiFi] IP: %s\n", WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println("[WiFi] Falha ao criar Access Point!");
    }
}

void setupWebServer() {
    // Arquivos estáticos do LittleFS
    server.serveStatic("/style.css", LittleFS, "/style.css");
    server.serveStatic("/app.js", LittleFS, "/app.js");
    // Chart.js local — o modo AP não tem internet, então a lib não
    // pode vir de CDN; ela é servida do próprio LittleFS.
    server.serveStatic("/chart.umd.min.js", LittleFS, "/chart.umd.min.js");

    // Página principal — servir index.html do LittleFS
    server.on("/", HTTP_GET, []() {
        File file = LittleFS.open("/index.html", "r");
        if (file) {
            server.streamFile(file, "text/html");
            file.close();
        } else {
            server.send(404, "text/plain", "index.html nao encontrado no LittleFS");
        }
    });

    // API: Comando do motor (malha aberta)
    server.on("/api/motor", HTTP_POST, handleMotorCommand);

    // API: Leitura do encoder
    server.on("/api/encoder", HTTP_GET, handleEncoderRead);

    // API: Status do sistema
    server.on("/api/status", HTTP_GET, handleStatus);

    // API: PID
    server.on("/api/pid/config", HTTP_GET, handlePidConfig);
    server.on("/api/pid/config", HTTP_POST, handlePidConfig);
    server.on("/api/pid/start", HTTP_POST, handlePidStart);
    server.on("/api/pid/stop", HTTP_POST, handlePidStop);
    server.on("/api/pid/response", HTTP_GET, handlePidResponse);

    // API: Auto-tune (relay feedback) e regras de sintonia
    server.on("/api/pid/autotune", HTTP_GET, handleAutotune);
    server.on("/api/pid/autotune", HTTP_POST, handleAutotune);
    server.on("/api/pid/tuning", HTTP_GET, handlePidTuning);
    server.on("/api/pid/tuning/apply", HTTP_POST, handlePidTuningApply);

    // API: Controle de posição (ângulo)
    server.on("/api/position/config", HTTP_GET, handlePositionConfig);
    server.on("/api/position/config", HTTP_POST, handlePositionConfig);
    server.on("/api/position/start", HTTP_POST, handlePositionStart);
    server.on("/api/position/stop", HTTP_POST, handlePositionStop);
    server.on("/api/position/zero", HTTP_POST, handlePositionZero);

    // OTA com senha
    ElegantOTA.begin(&server, "admin", OTA_PASSWORD);
    ElegantOTA.setAutoReboot(true);

    server.begin();
    Serial.println("Servidor HTTP + OTA iniciado");
}

// =============================================================
// Handlers — malha aberta, encoder e status
// =============================================================

void handleMotorCommand() {
    if (!server.hasArg("direction") || !server.hasArg("speed")) {
        server.send(400, "application/json", "{\"error\":\"missing parameters\"}");
        return;
    }

    // Comando manual desliga PID, auto-tune e posição (modo comparação)
    pidStop();
    autotuneCancel();
    posStop();

    String direction = server.arg("direction");
    int speed = server.arg("speed").toInt();

    if (direction == "forward") {
        motorSetDirection(MOTOR_DIR_FORWARD);
        motorSetSpeed(speed);
    } else if (direction == "reverse") {
        motorSetDirection(MOTOR_DIR_REVERSE);
        motorSetSpeed(speed);
    } else if (direction == "stop") {
        motorSetSpeed(0);
    } else {
        server.send(400, "application/json", "{\"error\":\"invalid direction\"}");
        return;
    }

    String json = "{\"direction\":\"" + direction + "\",\"speed\":" + String(speed) + "}";
    server.send(200, "application/json", json);
}

void handleEncoderRead() {
    long pulses = encoderGetPulses();
    float rpm = encoderGetRPM();
    float angle = encoderGetAngle();

    String json = "{\"pulses\":" + String(pulses) + ",\"rpm\":" + String(rpm, 1) +
                  ",\"angle\":" + String(angle, 1) + "}";
    server.send(200, "application/json", json);
}

void handleStatus() {
    String json = "{";
    json += "\"version\":\"" FIRMWARE_VERSION "\",";
    json += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"stations\":" + String(WiFi.softAPgetStationNum()) + ",";
    json += "\"ssid\":\"" + String(WIFI_SSID) + "\",";
    json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"uptime_ms\":" + String(millis()) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"angle\":" + String(encoderGetAngle(), 1) + ",";
    json += "\"mode\":\"";
    if (autotuneIsRunning()) json += "autotune";
    else if (posIsRunning()) json += "position";
    else if (pidIsRunning()) json += "pid";
    else json += "open-loop";
    json += "\"";
    json += "}";
    server.send(200, "application/json", json);
}

// =============================================================
// Handlers — PID
// =============================================================

String pidConfigToJson() {
    String json = "{";
    json += "\"kp\":" + String(pidGetKp(), 4) + ",";
    json += "\"ki\":" + String(pidGetKi(), 4) + ",";
    json += "\"kd\":" + String(pidGetKd(), 4) + ",";
    json += "\"setpoint\":" + String(pidGetSetpoint(), 1) + ",";
    json += "\"mode\":\"";
    json += pidIsRunning() ? "pid" : "open-loop";
    json += "\"";
    json += "}";
    return json;
}

void handlePidConfig() {
    if (server.method() == HTTP_GET) {
        server.send(200, "application/json", pidConfigToJson());
        return;
    }

    // POST: configura ganhos e setpoint (setpoint em RPM)
    if (!server.hasArg("kp") || !server.hasArg("ki") || !server.hasArg("kd")) {
        server.send(400, "application/json", "{\"error\":\"missing gains\"}");
        return;
    }

    float kp = server.arg("kp").toFloat();
    float ki = server.arg("ki").toFloat();
    float kd = server.arg("kd").toFloat();
    float setpoint = server.hasArg("setpoint") ? server.arg("setpoint").toFloat()
                                               : pidGetSetpoint();

    if (setpoint < 0.0 || setpoint > PID_MAX_SETPOINT) {
        server.send(400, "application/json", "{\"error\":\"invalid setpoint\"}");
        return;
    }

    pidConfigure(kp, ki, kd, setpoint);
    server.send(200, "application/json", pidConfigToJson());
}

void handlePidStart() {
    motorSetDirection(MOTOR_DIR_FORWARD);
    posStop();               // posição e velocidade não rodam juntos
    autotuneCancel();
    responseUnit = "rpm";
    resetResponseBuffer();
    pidStart();
    server.send(200, "application/json", "{\"ok\":true,\"mode\":\"pid\"}");
}

void handlePidStop() {
    pidStop();
    motorSetSpeed(0);
    motorSetDirection(MOTOR_DIR_STOP);
    server.send(200, "application/json", "{\"ok\":true,\"mode\":\"open-loop\"}");
}

void handlePidResponse() {
    // Em modo posição o alvo é o ângulo; em velocidade, o RPM.
    float sp = (strcmp(responseUnit, "deg") == 0) ? posGetTarget()
                                                  : pidGetSetpoint();

    String json = "{\"data\":[";
    for (int i = 0; i < responseLen; i++) {
        if (i > 0) json += ",";
        float t = (float)responseBuffer[i].tMs / 1000.0;
        json += "[" + String(t, 3) + "," +
                String(sp, 1) + "," +
                String(responseBuffer[i].value, 1) + "]";
    }
    json += "],\"setpoint\":" + String(sp, 1);
    json += ",\"unit\":\"" + String(responseUnit) + "\"";
    json += "}";
    server.send(200, "application/json", json);
}

// =============================================================
// Handlers — Auto-tune (relay feedback)
// =============================================================

void handleAutotune() {
    if (server.method() == HTTP_GET) {
        // Status do experimento
        String json = "{\"status\":\"";
        if (autotuneIsRunning()) json += "running";
        else if (autotuneIsDone()) json += "done";
        else if (autotuneIsFailed()) json += "failed";
        else json += "idle";
        json += "\",\"progress\":" + String(autotuneGetProgress());

        if (autotuneIsDone()) {
            float ku = autotuneGetKu();
            float tu = autotuneGetTu();
            json += ",\"results\":{\"ku\":" + String(ku, 3);
            json += ",\"tu\":" + String(tu, 3);
            json += ",\"zn\":" + gainsToJson(tuningZN(ku, tu));
            json += ",\"tl\":" + gainsToJson(tuningTL(ku, tu));
            json += ",\"cc\":" + gainsToJson(tuningCC(ku, tu));
            json += "}";
        }
        json += "}";
        server.send(200, "application/json", json);
        return;
    }

    // POST: inicia o experimento
    float relay = server.hasArg("relay_amplitude")
                      ? server.arg("relay_amplitude").toFloat()
                      : (float)AUTOTUNE_DEFAULT_RELAY;
    float bias = server.hasArg("bias")
                     ? server.arg("bias").toFloat()
                     : (float)AUTOTUNE_DEFAULT_BIAS;
    int cycles = server.hasArg("cycles")
                     ? server.arg("cycles").toInt()
                     : AUTOTUNE_DEFAULT_CYCLES;

    // Setpoint: argumento explícito > último setpoint do PID > padrão
    float setpoint;
    if (server.hasArg("setpoint")) {
        setpoint = server.arg("setpoint").toFloat();
    } else if (pidGetSetpoint() > 0.0) {
        setpoint = pidGetSetpoint();
    } else {
        setpoint = AUTOTUNE_DEFAULT_SETPOINT;
    }

    if (relay < 5.0 || relay > 100.0 || bias < 0.0 || bias > AUTOTUNE_BIAS_MAX ||
        cycles < 1 || cycles > 8 || setpoint <= 0.0 || setpoint > PID_MAX_SETPOINT) {
        server.send(400, "application/json", "{\"error\":\"invalid autotune parameters\"}");
        return;
    }

    pidStop();    // relay assume o motor
    posStop();
    responseUnit = "rpm";
    resetResponseBuffer();
    autotuneStart(relay, bias, cycles, setpoint);

    String json = "{\"ok\":true,\"relay\":" + String(relay, 1);
    json += ",\"bias\":" + String(bias, 1);
    json += ",\"cycles\":" + String(cycles);
    json += ",\"setpoint\":" + String(setpoint, 1) + "}";
    server.send(200, "application/json", json);
}

void handlePidTuning() {
    if (!autotuneIsDone()) {
        server.send(404, "application/json", "{\"error\":\"run autotune first\"}");
        return;
    }

    float ku = autotuneGetKu();
    float tu = autotuneGetTu();
    String json = "{\"ku\":" + String(ku, 3) + ",\"tu\":" + String(tu, 3);
    json += ",\"zn\":" + gainsToJson(tuningZN(ku, tu));
    json += ",\"tl\":" + gainsToJson(tuningTL(ku, tu));
    json += ",\"cc\":" + gainsToJson(tuningCC(ku, tu));
    json += "}";
    server.send(200, "application/json", json);
}

void handlePidTuningApply() {
    if (!server.hasArg("method")) {
        server.send(400, "application/json", "{\"error\":\"missing method\"}");
        return;
    }

    String method = server.arg("method");
    method.toUpperCase();

    if (!autotuneIsDone()) {
        server.send(404, "application/json", "{\"error\":\"run autotune first\"}");
        return;
    }

    float ku = autotuneGetKu();
    float tu = autotuneGetTu();
    PidGains g;

    if (method == "ZN") {
        g = tuningZN(ku, tu);
    } else if (method == "TL") {
        g = tuningTL(ku, tu);
    } else if (method == "CC") {
        g = tuningCC(ku, tu);
    } else {
        server.send(400, "application/json", "{\"error\":\"invalid method\"}");
        return;
    }

    pidConfigure(g.kp, g.ki, g.kd, pidGetSetpoint());
    server.send(200, "application/json", gainsToJson(g));
}

// =============================================================
// Handlers — Controle de posição (ângulo)
// =============================================================

String posConfigToJson() {
    String json = "{";
    json += "\"kp\":" + String(posGetKp(), 4) + ",";
    json += "\"ki\":" + String(posGetKi(), 4) + ",";
    json += "\"kd\":" + String(posGetKd(), 4) + ",";
    json += "\"target\":" + String(posGetTarget(), 1) + ",";
    json += "\"angle\":" + String(encoderGetAngle(), 1) + ",";
    json += "\"mode\":\"";
    json += posIsRunning() ? "position" : "open-loop";
    json += "\"";
    json += "}";
    return json;
}

void handlePositionConfig() {
    if (server.method() == HTTP_GET) {
        server.send(200, "application/json", posConfigToJson());
        return;
    }

    // POST: configura ganhos e alvo (alvo em graus)
    if (!server.hasArg("kp") || !server.hasArg("ki") || !server.hasArg("kd")) {
        server.send(400, "application/json", "{\"error\":\"missing gains\"}");
        return;
    }

    float kp = server.arg("kp").toFloat();
    float ki = server.arg("ki").toFloat();
    float kd = server.arg("kd").toFloat();
    float target = server.hasArg("target") ? server.arg("target").toFloat()
                                           : posGetTarget();

    if (target < -POS_MAX_TARGET_DEG || target > POS_MAX_TARGET_DEG) {
        server.send(400, "application/json", "{\"error\":\"invalid target\"}");
        return;
    }

    posConfigure(kp, ki, kd, target);
    server.send(200, "application/json", posConfigToJson());
}

void handlePositionStart() {
    // Alvo opcional no POST (permite iniciar já com novo ângulo)
    if (server.hasArg("target")) {
        float target = server.arg("target").toFloat();
        if (target >= -POS_MAX_TARGET_DEG && target <= POS_MAX_TARGET_DEG) {
            posConfigure(posGetKp(), posGetKi(), posGetKd(), target);
        }
    }
    pidStop();               // posição e velocidade não rodam juntos
    autotuneCancel();
    responseUnit = "deg";
    resetResponseBuffer();
    posStart();
    server.send(200, "application/json", posConfigToJson());
}

void handlePositionStop() {
    posStop();
    motorSetSpeed(0);
    motorSetDirection(MOTOR_DIR_STOP);
    server.send(200, "application/json", "{\"ok\":true,\"mode\":\"open-loop\"}");
}

void handlePositionZero() {
    // Define a posição atual como origem (0°). Para o controle por
    // segurança: zerar a referência com o motor girando mudaria o alvo
    // de forma abrupta.
    posStop();
    motorSetSpeed(0);
    motorSetDirection(MOTOR_DIR_STOP);
    encoderReset();
    server.send(200, "application/json",
                "{\"ok\":true,\"angle\":0}");
}

// =============================================================
// Helpers
// =============================================================

String gainsToJson(const PidGains& g) {
    String json = "{\"kp\":" + String(g.kp, 3);
    json += ",\"ki\":" + String(g.ki, 3);
    json += ",\"kd\":" + String(g.kd, 3) + "}";
    return json;
}
