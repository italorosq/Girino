# Guia do Firmware — Girino

## Compilação

### Com PlatformIO CLI

```bash
cd firmware

# Compilar (verificar erros)
pio run

# Compilar e gravar via USB
pio run -t upload

# Compilar, gravar e abrir monitor serial
pio run -t upload && pio device monitor
```

### Com VSCode + PlatformIO

1. Abra a pasta `firmware/` no VSCode
2. Clique no ícone ✓ (Build) na barra inferior
3. Clique na seta → (Upload) para gravar

## Configuração

### Variáveis de Ambiente (.env)

O firmware usa credenciais WiFi injetadas via build flags. Configure no arquivo `.env`:

```ini
WIFI_SSID="sua_rede_wifi"
WIFI_PASSWORD="sua_senha_wifi"
OTA_PASSWORD="sua_senha_ota"
```

### Configurações do Hardware (config.h)

| Constante | Default | Descrição |
|---|---|---|
| `BAUD_RATE` | 115200 | Velocidade serial |
| `MOTOR_PWM_FREQ` | 1000 | Frequência PWM (Hz) |
| `MOTOR_PWM_RANGE` | 1023 | Resolução PWM (10-bit) |
| `ENCODER_PPR` | 600 | Pulsos por revolução |
| `ENCODER_SAMPLE_MS` | 100 | Intervalo de amostragem do RPM |

### Pinagem (pins.h)

Ver mapeamento completo em [`firmware/include/pins.h`](../firmware/include/pins.h).

## Estrutura do Código

```
firmware/
├── src/
│   ├── main.cpp            # Setup, loop, WiFi, web server, API
│   ├── motor_control.cpp   # Controle PWM do motor
│   └── encoder.cpp         # Leitura do encoder via interrupção
├── include/
│   ├── pins.h              # GPIOs (centralizado — alterar aqui)
│   ├── config.h            # Constantes e configurações
│   ├── motor_control.h     # Interface do módulo motor
│   └── encoder.h           # Interface do módulo encoder
├── platformio.ini          # Configuração PlatformIO
├── .env.example            # Template de credenciais
└── README.md
```

## Monitor Serial

```bash
# Abrir monitor serial
pio device monitor

# Filtrar saída (ex: apenas encoder)
pio device monitor --filter direct
```

Atalhos:
- `Ctrl+]` — Sair do monitor
- `Ctrl+T` — Menu do monitor

## API REST

Ver documentação completa em [`firmware/README.md`](../firmware/README.md#api-rest).
