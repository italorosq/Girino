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

#include "pins.h"
#include "config.h"
#include "motor_control.h"
#include "encoder.h"

// --- Web Server na porta 80 ---
ESP8266WebServer server(80);

// --- Protótipos ---
void setupWiFi();
void setupWebServer();
void handleRoot();
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
    // Página principal
    server.on("/", HTTP_GET, handleRoot);

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

void handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Girino — Controle de Motor</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 600px; margin: 40px auto; padding: 0 20px; }
        h1 { color: #333; }
        .control { margin: 20px 0; padding: 20px; border: 1px solid #ddd; border-radius: 8px; }
        label { display: block; margin: 10px 0 5px; font-weight: bold; }
        input[type="range"] { width: 100%; }
        button { padding: 10px 20px; margin: 5px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; }
        .btn-on { background: #4CAF50; color: white; }
        .btn-off { background: #f44336; color: white; }
        .status { background: #f5f5f5; padding: 15px; border-radius: 8px; font-family: monospace; }
    </style>
</head>
<body>
    <h1>🔧 Girino</h1>
    <p>Plataforma didática — Controle de Motor DC</p>

    <div class="control">
        <h3>Controle do Motor</h3>
        <label>Velocidade (PWM): <span id="speedVal">0</span>%</label>
        <input type="range" id="speed" min="0" max="100" value="0">
        <br>
        <button class="btn-on" onclick="motorCmd('forward')">▶ Sentido Horário</button>
        <button class="btn-off" onclick="motorCmd('reverse')">◀ Sentido Anti-horário</button>
        <button class="btn-off" onclick="motorCmd('stop')">⏹ Parar</button>
    </div>

    <div class="status">
        <h3>Encoder</h3>
        <p>Pulsos: <span id="pulses">0</span></p>
        <p>Velocidade (RPM): <span id="rpm">0</span></p>
        <p><small>Atualiza a cada 500ms</small></p>
    </div>

    <p><a href="/update">🔄 Atualização OTA</a></p>

    <script>
        const speedSlider = document.getElementById('speed');
        const speedVal = document.getElementById('speedVal');

        speedSlider.oninput = () => { speedVal.textContent = speedSlider.value; };

        function motorCmd(dir) {
            fetch('/api/motor', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'direction=' + dir + '&speed=' + speedSlider.value
            }).then(r => r.json()).then(d => console.log(d));
        }

        setInterval(() => {
            fetch('/api/encoder').then(r => r.json()).then(d => {
                document.getElementById('pulses').textContent = d.pulses;
                document.getElementById('rpm').textContent = d.rpm.toFixed(1);
            });
        }, 500);

        setInterval(() => {
            fetch('/api/status').then(r => r.json()).then(d => {
                // Atualizar status se necessário
            });
        }, 2000);
    </script>
</body>
</html>
)rawliteral";

    server.send(200, "text/html", html);
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
