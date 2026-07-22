/**
 * Girino — Plataforma didática de controle de motor DC
 * LaRA — Laboratório de Robótica e Automação
 *
 * Firmware principal para ESP8266 WROOM.
 * Integra controle PWM de motor, leitura de encoder incremental
 * e atualização OTA via Wi-Fi.
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

// --- Web Server na porta 80 ---
ESP8266WebServer server(80);

// --- Protótipos ---
void setupWiFi();
void setupWebServer();
void handleMotorCommand();
void handleEncoderRead();
void handleStatus();

// =============================================================
// SETUP
// =============================================================
void setup() {
    Serial.begin(BAUD_RATE);
    Serial.println();
    Serial.println("=== Girino v0.1.0 ===");
    Serial.println("LaRA — Laboratório de Robótica e Automação");

    // Inicializar módulos
    motorControlInit();
    encoderInit();

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

    Serial.println("Sistema inicializado com sucesso!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("OTA: http://");
    Serial.println(WiFi.localIP().toString() + "/update");
}

// =============================================================
// LOOP
// =============================================================
void loop() {
    server.handleClient();
    ElegantOTA.loop();
    encoderUpdate();
}

// =============================================================
// Implementações
// =============================================================

void setupWiFi() {
    Serial.print("Conectando ao WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_TIMEOUT_MS / 500) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" Conectado!");
    } else {
        Serial.println(" Falha na conexão WiFi");
    }
}

void setupWebServer() {
    // Arquivos estáticos do LittleFS
    server.serveStatic("/style.css", LittleFS, "/style.css");
    server.serveStatic("/app.js", LittleFS, "/app.js");

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

    // API: Comando do motor
    server.on("/api/motor", HTTP_POST, handleMotorCommand);

    // API: Leitura do encoder
    server.on("/api/encoder", HTTP_GET, handleEncoderRead);

    // API: Status do sistema
    server.on("/api/status", HTTP_GET, handleStatus);

    // OTA com senha
    ElegantOTA.begin(&server, OTA_PASSWORD);
    ElegantOTA.setAutoReboot(true);

    server.begin();
    Serial.println("Servidor HTTP + OTA iniciado");
}

void handleMotorCommand() {
    if (!server.hasArg("direction") || !server.hasArg("speed")) {
        server.send(400, "application/json", "{\"error\":\"missing parameters\"}");
        return;
    }

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

    String json = "{\"pulses\":" + String(pulses) + ",\"rpm\":" + String(rpm, 1) + "}";
    server.send(200, "application/json", json);
}

void handleStatus() {
    String json = "{";
    json += "\"version\":\"0.1.0\",";
    json += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"uptime_ms\":" + String(millis()) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap());
    json += "}";
    server.send(200, "application/json", json);
}
