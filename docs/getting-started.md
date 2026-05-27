# Guia de Início Rápido — Girino

## Visão Geral

Este guia orienta você desde a montagem do hardware até o primeiro controle do motor via interface web.

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
| Módulo driver motor DC | 1 | Baseado em MOSFET |
| Módulo conversão de tensão | 1 | Para alimentação do circuito |
| Motor DC | 1 | Qualquer motor DC pequeno |
| Fonte de alimentação | 1 | Compatível com o motor e circuito |
| Cabos jumper | varios | Para conexões |
| Protoboard | 1 | Opcional — PCB recomendada |

## Passo 1: Montagem do Hardware

Siga o guia detalhado em [hardware-assembly.md](hardware-assembly.md).

### Esquema de Ligação Rápido

```
ESP8266 WROOM
├── D1 (GPIO5)  ← Encoder A (fio amarelo)
├── D2 (GPIO4)  ← Encoder B (fio verde)
├── D5 (GPIO14) → Motor PWM
├── D6 (GPIO12) → Motor DIR IN1
├── D7 (GPIO13) → Motor DIR IN2
├── Vin          ← 5V (módulo conversão)
├── GND         ← GND comum
└── USB         ← Para gravação inicial
```

## Passo 2: Configurar o Firmware

```bash
# Clonar o repositório
git clone https://github.com/usuario/Girino.git
cd Girino/firmware

# Configurar credenciais WiFi
cp .env.example .env
# Editar .env com suas credenciais
nano .env
```

## Passo 3: Compilar e Gravar

```bash
# Conectar ESP8266 via USB

# Compilar e gravar
pio run -t upload

# Monitor serial (verificar saída)
pio device monitor
```

Você deverá ver no monitor serial:
```
=== Girino v0.1.0 ===
LaRA — Laboratório de Robótica e Automação
[Motor] Inicializado
[Encoder] Inicializado — LPD3806-600BM (600 PPR)
Conectando ao WiFi.... Conectado!
Servidor HTTP + OTA iniciado
Sistema inicializado com sucesso!
IP: 192.168.1.xxx
OTA: http://192.168.1.xxx/update
```

## Passo 4: Controlar via Web

1. Abra o navegador no endereço IP mostrado no serial
2. Use a interface web para controlar o motor
3. Acompanhe a leitura do encoder em tempo real

## Próximos Passos

- [Atualização OTA](ota-update-guide.md) — Atualizar firmware sem cabo USB
- [Exemplos didáticos](../examples/) — Aprender conceitos de controle
- [Montar a PCB](../hardware/) — Versão definitiva em placa de circuito
