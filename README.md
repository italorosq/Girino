# Girino

[![License: GPL-3.0](https://img.shields.io/badge/Firmware-GPL--3.0-blue.svg)](LICENSE)
[![License: CERN-OHL-S-2.0](https://img.shields.io/badge/Hardware-CERN--OHL--S--2.0-orange.svg)](LICENSE)
[![License: CC-BY-SA-4.0](https://img.shields.io/badge/Docs-CC--BY--SA--4.0-green.svg)](LICENSE)

Plataforma didática do **LaRA** (Laboratório de Robótica e Automação) para ensino de controle de motores DC. Integra microcontrolador com Wi-Fi, driver de potência L298N e encoder em uma bancada compacta para introdução prática à leitura de sensores, modulação PWM, controle PID e sintonia em malha fechada.

A interface web é embarcada no próprio ESP8266: o aluno conecta na rede do Girino e controla a bancada pelo navegador do celular ou notebook — sem instalar MATLAB, Python ou qualquer software, e sem depender de internet.

## Status do Projeto

| Componente | Status |
|---|---|
| Estrutura do repositório | ✅ Criada |
| Firmware (ESP8266) | ✅ Funcional — PID de velocidade/posição e auto-tune validados em bancada |
| Hardware (PCB — KiCad) | ✅ Projeto KiCad + arquivos de fabricação |
| Mecânico (caixa 3D) | ✅ Modelos STEP (caixa e tampa) |
| Documentação | ✅ |
| Exemplos didáticos | ✅ 5 exemplos |

## Funcionalidades

- **Access Point próprio** (`GIRINO_AP`, `http://192.168.4.1`) — funciona sem roteador e sem internet
- Controle PWM de motor DC via ESP8266 (4 kHz, 8 bits)
- Leitura de encoder incremental (LPD3806-600BM — 600 PPR)
- **Controle PID de velocidade (RPM) e de posição (graus)** em malha fechada
- **Auto-sintonia por relay feedback** (Åström-Hägglund) para velocidade e posição
- **Comparação de métodos de sintonia**: Ziegler-Nichols, Tyreus-Luyben e Cohen-Coon
- Interface web em 3 abas (Velocidade / Posição / Manual), com gráfico de resposta (Chart.js local) e exportação CSV
- **Console serial de comandos** — opera a bancada sem Wi-Fi
- **Auto-calibração do sentido** motor/encoder (fiação invertida corrigida em software)
- Atualização de firmware via OTA (ElegantOTA)
- Design compacto para bancada de laboratório

## Componentes de Hardware

| Componente | Descrição |
|---|---|
| ESP8266 WROOM | Microcontrolador com Wi-Fi (Access Point) |
| LPD3806-600BM | Encoder incremental, 600 pulsos/revolução (saída coletor aberto — pull-up 4,7k para 3,3 V) |
| L298N | Driver de potência do motor DC (ponte H) |
| Motor DC | Motor da bancada |
| Módulo conversão de tensão | Regulador de tensão para alimentação do circuito |

Ligações, pull-ups e montagem passo a passo em [`docs/hardware-assembly.md`](docs/hardware-assembly.md).

## Bancada Medida

| Grandeza | Valor |
|---|---|
| Zona morta do motor | ~60% de duty (abaixo disso o eixo não gira) |
| Região confiável | 70% de duty ≈ 400 RPM |
| Velocidade máxima | 100% ≈ 1950 RPM |
| Precisão de posição | ±2° (o erro residual de ~1–2° é folga mecânica da correia, não do PID) |

## Estrutura do Repositório

```
Girino/
├── firmware/          # Firmware ESP8266 (PlatformIO) + interface web (LittleFS em data/)
├── hardware/          # PCB KiCad, arquivos de fabricação (gerber/NC) e modelos 3D
├── mechanical/        # Peças 3D (em preparação)
├── docs/              # Documentação, pesquisa e infográfico
├── examples/          # Exemplos didáticos passo a passo
├── gallery/           # Fotos e renders do projeto
└── .github/           # Templates e CI/CD
```

## Início Rápido

Veja o guia completo em [`docs/getting-started.md`](docs/getting-started.md).

### Pré-requisitos

- [PlatformIO](https://platformio.org/) (extensão VSCode recomendada)
- Cabo USB para a gravação inicial do ESP8266
- Componentes e montagem: ver [`docs/hardware-assembly.md`](docs/hardware-assembly.md)

### Compilar e Gravar

```bash
cd firmware

# 1. Configurar o Access Point do Girino (SSID, senha e senha do OTA)
cp .env.example .env

# 2. Primeira gravação via USB: firmware + interface web (LittleFS)
pio run -t upload
pio run -t uploadfs

# 3. Conectar no Wi-Fi GIRINO_AP e abrir http://192.168.4.1

# 4. Atualizações seguintes via OTA (PC conectado ao GIRINO_AP)
pio run -t upload --upload-port 192.168.4.1
```

> **Importante:** `pio run -t upload` grava apenas o firmware. A interface web
> vive no LittleFS e é gravada com `pio run -t uploadfs` — obrigatório na
> primeira gravação e sempre que alterar arquivos de `firmware/data/`.

## Documentação

- [Guia de Início Rápido](docs/getting-started.md)
- [Montagem do Hardware](docs/hardware-assembly.md)
- [Guia do Firmware](firmware/README.md) — API REST, pinagem e console serial
- [Atualização OTA](docs/ota-update-guide.md)
- [Análise Comparativa](docs/analise-comparativa.md) — trabalhos relacionados e gap de pesquisa
- [Referências de Pesquisa](docs/referencias-pesquisa.md)
- [Plano PID](docs/plano-pid.md) — métodos de sintonia e arquitetura
- [Resumo UERJ Sem Muros](docs/resumo-uerj-sem-muros.md) — divulgação do projeto

## Exemplos Didáticos

| # | Exemplo | Descrição |
|---|---|---|
| 01 | [Controle Básico de Motor](examples/01-basic-motor-control/) | PWM para controlar velocidade do motor |
| 02 | [Leitura de Encoder](examples/02-encoder-reading/) | Ler pulsos e calcular velocidade |
| 03 | [Controle PID](examples/03-pid-speed-control/) | Malha fechada de velocidade |
| 04 | [OTA Web Update](examples/04-ota-web-update/) | Atualizar firmware via Wi-Fi |
| 05 | [PID Web Test](examples/05-pid-web-test/) | Teste prático de PID com painel web e gráfico |

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

Desenvolvido no **LaRA — Laboratório de Robótica e Automação** (UERJ).

## Agradecimentos

- Comunidade open-source ESP8266
- [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA)
- [PlatformIO](https://platformio.org/)
