# Firmware — Girino

Firmware para ESP8266 WROOM com controle PWM de motor DC, leitura de encoder e OTA.

## Requisitos

- [PlatformIO](https://platformio.org/) (CLI ou extensão VSCode)
- Cabo USB para gravação inicial
- Python 3 (para PlatformIO)

## Configuração Inicial

```bash
# 1. Copiar template de credenciais
cp .env.example .env

# 2. Editar com suas credenciais WiFi
#    WIFI_SSID="sua_rede"
#    WIFI_PASSWORD="sua_senha"
#    OTA_PASSWORD="senha_para_ota"
nano .env
```

## Compilar e Gravar

### Primeira gravação (via USB)

```bash
# Compilar
pio run

# Compilar e gravar
pio run -t upload

# Monitor serial (Ctrl+] para sair)
pio device monitor
```

### Atualizações subsequentes (via OTA)

Após a primeira gravação, o dispositivo fica acessível na rede local.

```bash
# Via mDNS (se suportado pelo seu OS)
pio run -t upload --upload-port girino.local

# Via IP direto
pio run -t upload --upload-port 192.168.1.100
```

Ou pela interface web: `http://<IP_DO_ESP8266>/update`

## Arquitetura do Firmware

```
src/
├── main.cpp            # Loop principal, WiFi, web server
├── motor_control.cpp   # Controle PWM do motor
└── encoder.cpp         # Leitura do encoder via interrupção

include/
├── pins.h              # Mapeamento de GPIOs (centralizado)
├── config.h            # Constantes e configurações
├── motor_control.h     # Interface do módulo motor
└── encoder.h           # Interface do módulo encoder
```

## API REST

| Endpoint | Método | Descrição |
|---|---|---|
| `/` | GET | Interface web de controle |
| `/api/motor` | POST | Comando do motor (`direction`, `speed`) |
| `/api/encoder` | GET | Leitura atual do encoder (`pulses`, `rpm`) |
| `/api/status` | GET | Status do sistema (versão, heap, uptime) |
| `/update` | GET | Interface OTA (ElegantOTA) |

### Exemplos de uso da API

```bash
# Ligar motor em 50% sentido horário
curl -X POST http://192.168.1.100/api/motor -d "direction=forward&speed=50"

# Parar motor
curl -X POST http://192.168.1.100/api/motor -d "direction=stop&speed=0"

# Ler encoder
curl http://192.168.1.100/api/encoder

# Status do sistema
curl http://192.168.1.100/api/status
```

## Pinagem

| GPIO | Pino NodeMCU | Função |
|---|---|---|
| GPIO5 | D1 | Encoder Canal A |
| GPIO4 | D2 | Encoder Canal B |
| GPIO14 | D5 | Motor PWM |
| GPIO12 | D6 | Motor Direção IN1 |
| GPIO13 | D7 | Motor Direção IN2 |
| GPIO2 | LED | LED onboard (debug) |

## Dependências

| Biblioteca | Versão | Descrição |
|---|---|---|
| ElegantOTA | ^3.1.5 | Interface web para atualização OTA |
| ESP8266WebServer | ^1.0 | Servidor HTTP embutido |
