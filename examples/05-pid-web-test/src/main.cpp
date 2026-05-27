#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <math.h>

// 1) Configuracoes da rede Wi-Fi (edite antes de gravar)
const char *ssid = "nome_do_seu_wifi";
const char *password = "senha_do_seu_wifi";

// 2) Pinagem de referencia (igual ao codigo enviado)
const uint8_t PIN_ENC_A = 5;   // D1
const uint8_t PIN_ENC_B = 4;   // D2
const uint8_t PIN_IN3 = 14;    // D5
const uint8_t PIN_IN4 = 12;    // D6
const uint8_t PIN_ENB = 13;    // D7

// 3) Variaveis do encoder
volatile long posicaoAtual = 0;

// 4) Variaveis do PID
double kp = 1.0;
double ki = 0.0;
double kd = 0.0;
double setpoint = 0.0;
double erroAcumulado = 0.0;
double erroAnterior = 0.0;
unsigned long tempoAnterior = 0;

const unsigned long CONTROL_INTERVAL_MS = 10;
const double INTEGRAL_LIMIT = 50000.0;

ESP8266WebServer server(80);

long lerPosicaoComSeguranca() {
  noInterrupts();
  long valor = posicaoAtual;
  interrupts();
  return valor;
}

void ICACHE_RAM_ATTR lerEncoder() {
  if (digitalRead(PIN_ENC_B) > 0) {
    posicaoAtual++;
  } else {
    posicaoAtual--;
  }
}

const char painelHtml[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="pt-BR">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Girino - Teste PID Web</title>
  <style>
    :root {
      --bg: #0f1a1f;
      --panel: #13252e;
      --line: #1f3c4a;
      --text: #ecf2f4;
      --accent: #f2c94c;
      --motor: #56ccf2;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      font-family: "Trebuchet MS", "Verdana", sans-serif;
      color: var(--text);
      background:
        radial-gradient(circle at 15% 10%, #214150, transparent 35%),
        radial-gradient(circle at 80% 15%, #18313f, transparent 45%),
        var(--bg);
      display: grid;
      place-items: center;
      padding: 16px;
    }
    .wrap {
      width: min(960px, 100%);
      background: linear-gradient(180deg, rgba(255,255,255,0.03), rgba(255,255,255,0.01));
      border: 1px solid var(--line);
      border-radius: 14px;
      padding: 18px;
      box-shadow: 0 18px 40px rgba(0, 0, 0, 0.35);
    }
    h1 {
      margin: 0 0 6px;
      letter-spacing: 0.02em;
      font-size: clamp(20px, 2.8vw, 28px);
    }
    p {
      margin: 0 0 14px;
      opacity: 0.9;
    }
    .controles {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
      gap: 10px;
      margin-bottom: 14px;
    }
    .campo {
      background: var(--panel);
      border: 1px solid var(--line);
      border-radius: 10px;
      padding: 10px;
    }
    .campo label {
      display: block;
      font-size: 12px;
      margin-bottom: 6px;
      opacity: 0.9;
    }
    .campo input {
      width: 100%;
      border: 1px solid #365969;
      background: #0f2028;
      color: var(--text);
      border-radius: 8px;
      padding: 7px;
      font-size: 14px;
    }
    .acoes {
      display: flex;
      gap: 8px;
      align-items: end;
    }
    button {
      border: 0;
      border-radius: 10px;
      padding: 10px 14px;
      font-weight: 700;
      cursor: pointer;
      color: #05232b;
      background: linear-gradient(180deg, #8bd4ea, #4db9dd);
      transition: transform 120ms ease, filter 120ms ease;
      width: 100%;
    }
    button:hover { filter: brightness(1.05); }
    button:active { transform: translateY(1px); }
    .status {
      display: flex;
      gap: 16px;
      flex-wrap: wrap;
      margin: 8px 0 12px;
      font-size: 14px;
    }
    .status code { color: var(--accent); }
    canvas {
      width: 100%;
      height: auto;
      aspect-ratio: 2 / 1;
      border-radius: 10px;
      border: 1px solid var(--line);
      background: #0c171c;
      display: block;
    }
  </style>
</head>
<body>
  <main class="wrap">
    <h1>Teste PID com grafico em tempo real</h1>
    <p>Referencia de pinagem: D1/D2 (encoder), D5/D6 (direcao), D7 (PWM).</p>

    <section class="controles">
      <div class="campo">
        <label for="sp">Setpoint</label>
        <input type="number" id="sp" value="0" step="1">
      </div>
      <div class="campo">
        <label for="kp">Kp</label>
        <input type="number" id="kp" value="1.0" step="0.1">
      </div>
      <div class="campo">
        <label for="ki">Ki</label>
        <input type="number" id="ki" value="0.0" step="0.01">
      </div>
      <div class="campo">
        <label for="kd">Kd</label>
        <input type="number" id="kd" value="0.0" step="0.1">
      </div>
      <div class="acoes">
        <button onclick="enviar()">Atualizar controle</button>
      </div>
    </section>

    <section class="status">
      <div>Posicao: <code id="posicaoAtual">0</code></div>
      <div>Setpoint: <code id="setpointAtual">0</code></div>
    </section>

    <canvas id="grafico" width="800" height="400"></canvas>
  </main>

  <script>
    const canvas = document.getElementById('grafico');
    const ctx = canvas.getContext('2d');
    const dadosPos = [];
    const dadosSp = [];
    const maxPontos = 120;

    function toY(valor, minimo, maximo, altura) {
      return altura - ((valor - minimo) / (maximo - minimo)) * altura;
    }

    function desenhar() {
      const w = canvas.width;
      const h = canvas.height;
      ctx.clearRect(0, 0, w, h);

      if (dadosPos.length < 2) return;

      let minimo = Infinity;
      let maximo = -Infinity;

      for (const v of dadosPos) {
        if (v < minimo) minimo = v;
        if (v > maximo) maximo = v;
      }
      for (const v of dadosSp) {
        if (v < minimo) minimo = v;
        if (v > maximo) maximo = v;
      }

      if (minimo === maximo) {
        minimo -= 1;
        maximo += 1;
      }

      const passoX = w / (maxPontos - 1);

      ctx.strokeStyle = 'rgba(255,255,255,0.08)';
      ctx.lineWidth = 1;
      for (let i = 1; i < 5; i++) {
        const y = (h / 5) * i;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(w, y);
        ctx.stroke();
      }

      ctx.strokeStyle = '#f2c94c';
      ctx.lineWidth = 2;
      ctx.beginPath();
      for (let i = 0; i < dadosSp.length; i++) {
        const x = i * passoX;
        const y = toY(dadosSp[i], minimo, maximo, h);
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
      }
      ctx.stroke();

      ctx.strokeStyle = '#56ccf2';
      ctx.lineWidth = 2;
      ctx.beginPath();
      for (let i = 0; i < dadosPos.length; i++) {
        const x = i * passoX;
        const y = toY(dadosPos[i], minimo, maximo, h);
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
      }
      ctx.stroke();
    }

    function enviar() {
      const sp = document.getElementById('sp').value;
      const kp = document.getElementById('kp').value;
      const ki = document.getElementById('ki').value;
      const kd = document.getElementById('kd').value;
      fetch(`/set?sp=${sp}&kp=${kp}&ki=${ki}&kd=${kd}`);
    }

    setInterval(() => {
      fetch('/status')
        .then((r) => r.json())
        .then((d) => {
          dadosPos.push(d.posicao);
          dadosSp.push(d.setpoint);
          if (dadosPos.length > maxPontos) {
            dadosPos.shift();
            dadosSp.shift();
          }

          document.getElementById('posicaoAtual').textContent = d.posicao;
          document.getElementById('setpointAtual').textContent = Number(d.setpoint).toFixed(2);
          desenhar();
        });
    }, 100);
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== Girino - Exemplo 05 PID Web Test ===");

  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);
  pinMode(PIN_ENB, OUTPUT);

  analogWriteRange(255);
  analogWriteFreq(1000);

  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), lerEncoder, RISING);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado! IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", painelHtml);
  });

  server.on("/status", HTTP_GET, []() {
    long pos = lerPosicaoComSeguranca();
    String json = "{\"posicao\":" + String(pos) + ",\"setpoint\":" + String(setpoint, 2) + "}";
    server.send(200, "application/json", json);
  });

  server.on("/set", HTTP_GET, []() {
    if (server.hasArg("sp")) {
      setpoint = server.arg("sp").toFloat();
    }
    if (server.hasArg("kp")) {
      kp = server.arg("kp").toFloat();
    }
    if (server.hasArg("ki")) {
      ki = server.arg("ki").toFloat();
    }
    if (server.hasArg("kd")) {
      kd = server.arg("kd").toFloat();
    }

    erroAcumulado = 0.0;
    erroAnterior = 0.0;

    server.send(200, "text/plain", "atualizado");
  });

  server.begin();
  tempoAnterior = millis();
  Serial.println("Servidor web iniciado em http://<ip>/");
}

void loop() {
  server.handleClient();

  const unsigned long tempoAgora = millis();
  const unsigned long tempoPassado = tempoAgora - tempoAnterior;

  if (tempoPassado >= CONTROL_INTERVAL_MS) {
    const long pos = lerPosicaoComSeguranca();
    const double erro = setpoint - static_cast<double>(pos);
    const double dt = static_cast<double>(tempoPassado);

    erroAcumulado += erro * dt;
    if (erroAcumulado > INTEGRAL_LIMIT) {
      erroAcumulado = INTEGRAL_LIMIT;
    }
    if (erroAcumulado < -INTEGRAL_LIMIT) {
      erroAcumulado = -INTEGRAL_LIMIT;
    }

    const double taxaErro = (erro - erroAnterior) / dt;
    double forcaMotor = (kp * erro) + (ki * erroAcumulado) + (kd * taxaErro);

    if (forcaMotor > 255.0) {
      forcaMotor = 255.0;
    }
    if (forcaMotor < -255.0) {
      forcaMotor = -255.0;
    }

    if (forcaMotor > 0.0) {
      digitalWrite(PIN_IN3, HIGH);
      digitalWrite(PIN_IN4, LOW);
      analogWrite(PIN_ENB, static_cast<int>(forcaMotor));
    } else if (forcaMotor < 0.0) {
      digitalWrite(PIN_IN3, LOW);
      digitalWrite(PIN_IN4, HIGH);
      analogWrite(PIN_ENB, static_cast<int>(fabs(forcaMotor)));
    } else {
      digitalWrite(PIN_IN3, LOW);
      digitalWrite(PIN_IN4, LOW);
      analogWrite(PIN_ENB, 0);
    }

    erroAnterior = erro;
    tempoAnterior = tempoAgora;
  }
}
