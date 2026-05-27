# Girino

[![License: GPL-3.0](https://img.shields.io/badge/Firmware-GPL--3.0-blue.svg)](LICENSE)
[![License: CERN-OHL-S-2.0](https://img.shields.io/badge/Hardware-CERN--OHL--S--2.0-orange.svg)](LICENSE)
[![License: CC-BY-SA-4.0](https://img.shields.io/badge/Docs-CC--BY--SA--4.0-green.svg)](LICENSE)

Plataforma didática do **LaRA** (Laboratório de Robótica e Automação) para ensino de controle de motores DC. Integra microcontrolador, driver MOSFET e encoder em uma bancada compacta para introdução prática à leitura de sensores, modulação PWM e controle em malha fechada.

## Status do Projeto

| Componente | Status |
|---|---|
| Estrutura do repositório | ✅ Criada |
| Firmware (ESP8266) | 🚧 Em desenvolvimento |
| Hardware (PCB - KiCad) | 📋 Planejado |
| Mecânico (Caixa 3D) | 📋 Planejado |
| Documentação | 🚧 Em desenvolvimento |
| Exemplos didáticos | 📋 Planejado |

## Funcionalidades

- Controle PWM de motor DC via ESP8266
- Leitura de encoder incremental (LPD3806-600BM — 600 PPR)
- Atualização de firmware via OTA (Wi-Fi)
- Interface web para controle e monitoramento
- Controle PID de velocidade/posição
- Design compacto para bancada de laboratório

## Componentes de Hardware

| Componente | Descrição |
|---|---|
| ESP8266 WROOM | Microcontrolador com Wi-Fi |
| LPD3806-600BM | Encoder incremental, 600 pulsos/revolução |
| Módulo driver motor | Driver genérico MOSFET para motor DC |
| Módulo conversão tensão | Regulador de tensão para alimentação do circuito |

## Estrutura do Repositório

```
Girino/
├── firmware/          # Firmware ESP8266 (PlatformIO)
├── hardware/          # PCB designs (KiCad), gerbers, BOM
├── mechanical/        # Peças 3D (FreeCAD, STL, STEP)
├── docs/              # Documentação completa
├── examples/          # Exemplos didáticos passo a passo
├── gallery/           # Fotos e renders do projeto
└── .github/           # Templates e CI/CD
```

## Início Rápido

Veja o guia completo em [`docs/getting-started.md`](docs/getting-started.md).

### Pré-requisitos

- [PlatformIO](https://platformio.org/) (extensão VSCode recomendada)
- USB cable para gravação inicial do ESP8266
- Componentes listados no BOM (ver [`hardware/bom/`](hardware/bom/))

### Compilar e Gravar

```bash
cd firmware
# Configurar WiFi — copiar template e editar
cp .env.example .env

# Compilar e gravar via USB
pio run -t upload

# Após gravação inicial, usar OTA para atualizações
pio run -t upload --upload-port girino.local
```

## Documentação

- [Guia de Início Rápido](docs/getting-started.md)
- [Montagem do Hardware](docs/hardware-assembly.md)
- [Guia do Firmware](docs/firmware-guide.md)
- [Atualização OTA](docs/ota-update-guide.md)

## Exemplos Didáticos

| # | Exemplo | Descrição |
|---|---|---|
| 01 | [Controle Básico de Motor](examples/01-basic-motor-control/) | PWM para controlar velocidade do motor |
| 02 | [Leitura de Encoder](examples/02-encoder-reading/) | Ler pulsos e calcular velocidade |
| 03 | [Controle PID](examples/03-pid-speed-control/) | Malha fechada de velocidade |
| 04 | [OTA Web Update](examples/04-ota-web-update/) | Atualizar firmware via Wi-Fi |

## Contribuindo

Contribuições são bem-vindas! Veja o guia em [`CONTRIBUTING.md`](CONTRIBUTING.md).

## Licenças

Este projeto utiliza **tripla licença**:

| Tipo | Licença | Arquivo |
|---|---|---|
| Firmware (código) | **GPL-3.0** | [`LICENSE`](LICENSE) |
| Hardware (PCB, esquemático) | **CERN-OHL-S-2.0** | [`LICENSE`](LICENSE) |
| Documentação e imagens | **CC-BY-SA-4.0** | [`LICENSE`](LICENSE) |

## Equipe

Desenvolvido no **LaRA — Laboratório de Robótica e Automação**.

## Agradecimentos

- Comunidade open-source ESP8266
- [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA)
- [PlatformIO](https://platformio.org/)
