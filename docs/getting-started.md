# Guia de Início Rápido — Girino

## Visão Geral

Este guia orienta você desde a montagem do hardware até o primeiro controle do motor via interface web. O Girino cria a própria rede Wi-Fi (**Access Point**): não é preciso roteador nem internet — basta conectar no celular ou notebook e abrir a página de controle.

## Pré-requisitos

### Ferramentas

- Computador com [VSCode](https://code.visualstudio.com/) + extensão [PlatformIO](https://platformio.org/)
- Cabo USB micro-B
- Ferro de solda (para montagem do hardware)

### Componentes

| Componente | Qtd | Notas |
|---|---|---|
| ESP8266 WROOM | 1 | Módulo com antena PCB |
| Encoder LPD3806-600BM | 1 | 600 pulsos/revolução |
| Driver L298N | 1 | Ponte H para o motor DC |
| Motor DC | 1 | Qualquer motor DC pequeno |
| Módulo conversão de tensão | 1 | Para alimentação do circuito |
| Resistores 4,7k | 2 | Pull-up dos canais A/B do encoder para 3,3 V |
| Fonte de alimentação | 1 | Compatível com o motor e circuito |
| Cabos jumper | vários | Para conexões |
| Protoboard | 1 | Opcional — PCB recomendada |

## Passo 1: Montagem do Hardware

Siga o guia detalhado em [hardware-assembly.md](hardware-assembly.md).

### Esquema de Ligação Rápido

```
ESP8266 WROOM
├── D1 (GPIO5)  ← Encoder A (fio amarelo) + pull-up 4,7k para 3.3V
├── D2 (GPIO4)  ← Encoder B (fio verde)  + pull-up 4,7k para 3.3V
├── D5 (GPIO14) → Motor DIR IN3 (L298N)
├── D6 (GPIO12) → Motor DIR IN4 (L298N)
├── D7 (GPIO13) → Motor PWM ENB (L298N)
├── Vin          ← 5V (módulo conversão)
├── GND         ← GND comum
└── USB         ← Para gravação inicial
```

> O encoder LPD3806 tem saída em **coletor aberto**: sem os pull-ups externos de 4,7k (para 3,3 V, nunca 5 V) o ESP8266 não lê os pulsos. Detalhes e justificativa em [hardware-assembly.md](hardware-assembly.md).

## Passo 2: Configurar o Firmware

```bash
# Clonar o repositório
git clone https://github.com/LaRA-UERJ/Girino.git
cd Girino/firmware

# Copiar o template de configuração do Access Point
cp .env.example .env
nano .env
```

O `.env` **não** são credenciais de roteador — são as da rede que o próprio Girino cria:

| Variável | Padrão | Descrição |
|---|---|---|
| `WIFI_SSID` | `GIRINO_AP` | Nome da rede Wi-Fi criada pelo ESP8266 |
| `WIFI_PASSWORD` | `girino123` | Senha dessa rede (mínimo 8 caracteres; senha curta sobe rede aberta) |
| `OTA_PASSWORD` | `girino_ota` | Senha da página de atualização OTA |

## Passo 3: Compilar e Gravar

```bash
# Compilar (opcional, o upload já compila)
pio run

# Gravar o firmware via USB
pio run -t upload

# Gravar a interface web (LittleFS) — obrigatório na primeira gravação
pio run -t uploadfs

# Monitor serial (115200 baud; Ctrl+] para sair)
pio device monitor
```

Você deverá ver no monitor serial:

```
=== Girino v0.1.0 ===
LaRA — Laboratório de Robótica e Automação
[Motor] Inicializado
[Encoder] Inicializado — LPD3806-600BM (600 PPR)
[PID] Inicializado
[Pos] Controle de posicao inicializado (angulo)
[AutoTune] Inicializado
[WiFi] Access Point 'GIRINO_AP' ativo
[WiFi] IP: 192.168.4.1
[LittleFS] Filesystem montado
Servidor HTTP + OTA iniciado
[Console] Digite 'help' para os comandos (115200 baud)
Sistema inicializado com sucesso!
Pagina de controle: http://192.168.4.1
OTA: http://192.168.4.1/update
```

> Se a página abrir com **"index.html nao encontrado no LittleFS"**, faltou o `pio run -t uploadfs` (a interface web é gravada separadamente do firmware).

## Passo 4: Controlar via Web

1. No celular ou notebook, conecte na rede Wi-Fi **`GIRINO_AP`** (senha definida no `.env`)
2. Abra o navegador em **`http://192.168.4.1`**
3. Use as abas **Velocidade**, **Posição** ou **Manual**; o botão **PARAR** no cabeçalho desliga o motor a qualquer momento

## Alternativa sem Wi-Fi: Console Serial

Com o monitor serial aberto (`pio device monitor`), digite `help`. Os principais comandos:

| Comando | Descrição |
|---|---|
| `status` | modo, RPM, ângulo, pulsos, heap e IP |
| `motor <fwd\|rev\|stop> [0-100]` | comando manual do motor |
| `pid <kp> <ki> <kd> <sp>` / `pid start` | PID de velocidade (RPM) |
| `pos <kp> <ki> <kd> <graus>` / `pos start` / `pos zero` | PID de posição (ângulo) |
| `autotune <amp> <bias> <n> <sp>` | auto-sintonia da velocidade |
| `autotune pos <amp> <n> <graus>` | auto-sintonia da posição |
| `tune` | mostra Ku/Tu e ganhos ZN/TL/CC |

Lista completa em [`firmware/README.md`](../firmware/README.md).

## Atualizações via OTA

Com o computador conectado na rede do Girino:

```bash
# Atualizar o firmware sem cabo USB
pio run -t upload --upload-port 192.168.4.1
```

Ou use a interface web em `http://192.168.4.1/update`.

> **Atenção:** OTA grava apenas o firmware. Mudanças na interface web (`firmware/data/`) exigem `pio run -t uploadfs` via USB.

## Próximos Passos

- [Guia do Firmware](../firmware/README.md) — API REST, pinagem e console serial
- [Atualização OTA](ota-update-guide.md) — detalhes do ElegantOTA
- [Exemplos didáticos](../examples/) — aprender conceitos de controle
- [Hardware](../hardware/) — PCB em KiCad e arquivos de fabricação
