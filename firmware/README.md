# Firmware — Girino

Firmware para ESP8266 WROOM com controle PWM de motor DC, leitura de encoder e OTA.

## Requisitos

- [PlatformIO](https://platformio.org/) (CLI ou extensão VSCode)
- Cabo USB para gravação inicial
- Python 3 (para PlatformIO)

## Configuração Inicial

O Girino opera como **Access Point**: ele cria a própria rede Wi-Fi
(chamada `GIRINO_AP` por padrão) e os alunos conectam direto pelo
celular/notebook, sem precisar de roteador nem internet.

```bash
# 1. Copiar template de credenciais
cp .env.example .env

# 2. (Opcional) Ajustar nome e senha da rede criada pelo ESP:
#    WIFI_SSID="GIRINO_AP"
#    WIFI_PASSWORD="girino123"     # mínimo 8 caracteres
#    OTA_PASSWORD="senha_para_ota"
nano .env
```

Após conectar na rede do ESP, acesse `http://192.168.4.1`.

## Compilar e Gravar

### Primeira gravação (via USB)

```bash
# Compilar
pio run

# Compilar e gravar o firmware
pio run -t upload

# Gravar o filesystem (interface web em data/) — OBRIGATÓRIO na
# primeira gravação e sempre que alterar arquivos de data/
pio run -t uploadfs

# Monitor serial (Ctrl+] para sair)
pio device monitor
```

> **Importante:** `pio run -t upload` grava apenas o firmware. A interface
> web (`data/index.html`, `style.css`, `app.js`, `chart.umd.min.js`) vive
> no LittleFS e é gravada com `pio run -t uploadfs`. Se a página abrir com
> "index.html nao encontrado no LittleFS", falta o `uploadfs`.

### Console serial (controle sem Wi-Fi)

O firmware tem um console de comandos na própria porta serial — útil para
bancada, testes rápidos e quando não se quer usar o navegador. Abra o
monitor (`pio device monitor`) e digite `help`:

| Comando | Descrição |
|---|---|
| `help` | lista os comandos |
| `status` | modo, RPM, ângulo, pulsos, heap, IP |
| `enc` | leitura do encoder agora |
| `motor <fwd\|rev\|stop> [0-100]` | comando manual (cancela controladores) |
| `pid <kp> <ki> <kd> <sp>` | configura o PID de velocidade (RPM) |
| `pid start` / `pid stop` | liga/desliga a malha de velocidade |
| `pos <kp> <ki> <kd> <graus>` | configura o PID de posição |
| `pos start` / `pos stop` / `pos zero` | controle de posição / zera origem |
| `autotune <amp> <bias> <n> <sp>` | inicia o relay feedback (velocidade) |
| `autotune pos <amp> <n> <graus>` | inicia o relay feedback (posição, ±amp) |
| `autotune cancel` | cancela o experimento |
| `tune` | mostra Ku/Tu e ganhos ZN/TL/CC do último auto-tune |

Durante qualquer controlador ativo, a telemetria sai a 2 Hz no serial
(`[PID] SP/RPM/Out`, `[Pos] Alvo/Ângulo/Out`, `[AutoTune] RPM/relay`), e o
encoder reporta 1 Hz (`[Encoder] pulsos/p-s/rpm/ângulo`).

### Auto-calibração do sentido (fiação)

No primeiro movimento após o boot, o firmware aplica um pulso curto de
teste e observa a contagem do encoder:

- Contagem positiva → fiação normal (`[Motor] Sentido OK`)
- Contagem negativa → fiação invertida detectada e **corrigida em
  software** (`[Motor] Fiacao invertida detectada`) — não é preciso
  trocar fios: "horário" (FORWARD) sempre aumenta o ângulo medido

As malhas de velocidade e auto-tune usam |RPM| e funcionam com qualquer
polaridade; a calibração garante também o controle de posição.

### Atualizações subsequentes (via OTA)

Conecte o computador na rede do próprio ESP (`GIRINO_AP`) ou use a
interface web em `http://192.168.4.1/update`.

```bash
# Via OTA (computador conectado ao GIRINO_AP)
pio run -t upload --upload-port 192.168.4.1
```

> OTA e `uploadfs` não são a mesma coisa: OTA grava o **firmware**;
> alterações na interface web (pasta `data/`) exigem `pio run -t uploadfs`
> via USB.

## Arquitetura do Firmware

```
src/
├── main.cpp            # Loop principal, WiFi, web server, API REST
├── motor_control.cpp   # Controle PWM do motor
├── encoder.cpp         # Leitura do encoder via interrupção
├── pid.cpp             # Controle PID em malha fechada (50 Hz)
├── pid_autotune.cpp    # Auto-tune via relay feedback (Åström-Hägglund)
└── pid_tuning_rules.cpp # Regras de sintonia Ziegler-Nichols, Tyreus-Luyben e Cohen-Coon

include/
├── pins.h              # Mapeamento de GPIOs (centralizado)
├── config.h            # Constantes e configurações
├── motor_control.h     # Interface do módulo motor
├── encoder.h           # Interface do módulo encoder
├── pid.h               # Interface do módulo PID
├── pid_autotune.h      # Interface do auto-tune
└── pid_tuning_rules.h  # Interface das regras de sintonia
```

## API REST

| Endpoint | Método | Descrição |
|---|---|---|
| `/` | GET | Interface web de controle |
| `/api/motor` | POST | Comando do motor (`direction`, `speed`) |
| `/api/encoder` | GET | Leitura atual do encoder (`pulses`, `rpm`, `angle`) |
| `/api/status` | GET | Status (versão, heap, uptime, IP, clientes do AP, modo) |
| `/api/pid/config` | GET/POST | Configura/consulta ganhos e setpoint do PID de velocidade |
| `/api/pid/start` | POST | Inicia controle de velocidade em malha fechada |
| `/api/pid/stop` | POST | Para o PID, volta a malha aberta |
| `/api/pid/autotune` | GET/POST | Inicia/consulta experimento relay feedback (`plant=speed\|position`) |
| `/api/pid/tuning` | GET | Planta identificada (`plant`), Ku, Tu e sugestões `zn`/`tl`/`cc` |
| `/api/pid/tuning/apply` | POST | Aplica regra de sintonia (`method=ZN|TL|CC`) |
| `/api/pid/response` | GET | Última resposta ao degrau (tempo, setpoint, medida, `unit`) |
| `/api/position/config` | GET/POST | Configura/consulta ganhos e alvo do PID de posição (`target` em graus) |
| `/api/position/start` | POST | Inicia controle de posição (`target` opcional em graus) |
| `/api/position/stop` | POST | Para o controle de posição |
| `/api/position/zero` | POST | Define a posição atual como 0° (origem) |
| `/update` | GET | Interface OTA (ElegantOTA) |

### Exemplos de uso da API

```bash
# Ligar motor em 50% sentido horário (malha aberta)
curl -X POST http://192.168.4.1/api/motor -d "direction=forward&speed=50"

# Parar motor
curl -X POST http://192.168.4.1/api/motor -d "direction=stop&speed=0"

# Ler encoder
curl http://192.168.4.1/api/encoder

# Status do sistema
curl http://192.168.4.1/api/status

# Configurar PID (setpoint em RPM)
curl -X POST http://192.168.4.1/api/pid/config -d "kp=2.0&ki=5.0&kd=0.1&setpoint=100"

# Iniciar malha fechada
curl -X POST http://192.168.4.1/api/pid/start

# Iniciar auto-tune de VELOCIDADE (relay ±10% em torno de bias 90%, 3 ciclos, setpoint 100 RPM)
# O bias (pedestal) precisa vencer a zona morta do motor — o PWM mínimo
# que faz o eixo girar (medido nesta bancada: ~80%). Use `bias` entre esse
# valor e 95%, e `relay_amplitude` de forma que bias ± amplitude fique
# dentro de 0-100%.
curl -X POST http://192.168.4.1/api/pid/autotune -d "plant=speed&relay_amplitude=10&bias=90&cycles=3&setpoint=100"

# Iniciar auto-tune de POSIÇÃO (relé simétrico ±90% em torno do alvo 90°)
# O relé alterna horário/anti-horário; a amplitude precisa ficar acima da
# zona morta (~60%) para ambos os estados moverem o motor. Os ganhos
# resultantes estão em %/° (malha de posição).
curl -X POST http://192.168.4.1/api/pid/autotune -d "plant=position&relay_amplitude=90&cycles=3&target=90"

# Consultar resultado do auto-tune (inclui "plant": onde os ganhos se aplicam)
curl http://192.168.4.1/api/pid/autotune

# Aplicar Tyreus-Luyben
curl -X POST http://192.168.4.1/api/pid/tuning/apply -d "method=TL"

# Baixar resposta ao degrau
curl http://192.168.4.1/api/pid/response

# --- Controle de posição (ângulo) ---

# Zerar a posição atual (define origem 0°)
curl -X POST http://192.168.4.1/api/position/zero

# Configurar PID de posição (alvo 90°)
curl -X POST http://192.168.4.1/api/position/config -d "kp=1.0&ki=0.2&kd=0.05&target=90"

# Iniciar controle de posição
curl -X POST http://192.168.4.1/api/position/start

# Parar controle de posição
curl -X POST http://192.168.4.1/api/position/stop
```

## Pinagem

| GPIO | Pino NodeMCU | Função |
|---|---|---|
| GPIO5 | D1 | Encoder Canal A |
| GPIO4 | D2 | Encoder Canal B |
| GPIO14 | D5 | Motor Direção INB1 (alias IN3) |
| GPIO12 | D6 | Motor Direção INB2 (alias IN4) |
| GPIO13 | D7 | Motor PWM ENB |
| GPIO2 | LED | LED onboard (debug) |

> **Nota sobre o encoder:** o LPD3806 tem saída em coletor aberto — os
> canais A e B exigem resistores de pull-up externos de 4,7k para 3.3V
> (nunca 5V). Ver detalhes em [`docs/hardware-assembly.md`](../docs/hardware-assembly.md).

## Dependências

| Biblioteca | Versão | Descrição |
|---|---|---|
| ElegantOTA | ^3.1.5 | Interface web para atualização OTA |
| ESP8266WebServer | ^1.0 | Servidor HTTP embutido |
