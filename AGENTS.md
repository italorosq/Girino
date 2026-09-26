# AGENTS.md — Guia de Contexto para IA

Este arquivo fornece contexto essencial para agentes de IA trabalharem no projeto Girino. Leia antes de iniciar qualquer tarefa.

## Visão Geral

**Girino** é uma plataforma didática do **LaRA** (Laboratório de Robótica e Automação) para ensino de controle de motores DC em disciplinas de graduação. O projeto integra:

- **ESP8266 WROOM** como microcontrolador principal
- **Encoder LPD3806-600BM** (600 pulsos/revolução) para realimentação
- **Driver MOSFET genérico** para acionamento do motor DC
- **Módulo conversão de tensão** para alimentação do circuito
- **OTA via ElegantOTA** para atualização de firmware via Wi-Fi

O público-alvo são alunos de graduação. O código e a documentação devem ser claros e didáticos.

## Comandos Essenciais

| Comando | Descrição |
|---|---|
| `cd firmware && pio run` | Compilar o firmware |
| `cd firmware && pio run -t upload` | Compilar e gravar firmware via USB |
| `cd firmware && pio run -t uploadfs` | Gravar interface web (LittleFS) — obrigatório na 1ª gravação e após mudar `data/` |
| `cd firmware && pio run -t upload --upload-port 192.168.4.1` | Gravar firmware via OTA (PC conectado ao `GIRINO_AP`) |
| `cd firmware && pio device monitor` | Monitor serial (115200 baud) |

**Sempre execute `cd firmware && pio run` antes de commitar** para verificar que o firmware compila sem erros.

## Estrutura do Projeto

```
Girino/
├── firmware/               # Firmware ESP8266 (PlatformIO)
│   ├── src/                # Código-fonte (.cpp)
│   │   ├── main.cpp        # Loop principal, WiFi, web server, API REST
│   │   ├── motor_control.cpp  # Controle PWM do motor
│   │   └── encoder.cpp     # Leitura do encoder via interrupção GPIO
│   ├── include/            # Headers (.h)
│   │   ├── pins.h          # GPIOs centralizados — ALTERAR AQUI
│   │   ├── config.h        # Constantes (PPR, PWM freq, timeouts, etc.)
│   │   ├── motor_control.h # Interface do módulo motor
│   │   └── encoder.h       # Interface do módulo encoder
│   ├── lib/                # Bibliotecas locais
│   ├── test/               # Testes unitários
│   ├── platformio.ini      # Configuração PlatformIO
│   ├── data/               # Interface web (LittleFS): index.html, app.js, style.css, chart.umd.min.js
│   ├── scripts/            # Scripts de build (load_env.py — credenciais do .env)
│   ├── .env.example        # Template de credenciais (NÃO commitar .env)
│   └── README.md           # Docs do firmware
├── hardware/               # PCBs (KiCad)
│   ├── kicad/              # Projeto KiCad (.kicad_pro, .sch, .pcb)
│   ├── gerbers/            # Gerbers empacotados (.zip por versão)
│   ├── bom/                # Bill of Materials (CSV)
│   └── schematics/         # PDFs do esquemático
├── mechanical/             # Peças 3D (FreeCAD)
│   ├── cad/                # Arquivos-fonte FreeCAD (.FCStd)
│   ├── stl/                # STLs prontos para impressão
│   └── step/               # STEP para referência
├── docs/                   # Documentação técnica
│   ├── getting-started.md  # Setup completo
│   ├── hardware-assembly.md # Montagem do hardware
│   ├── firmware-guide.md   # Compilação e gravação
│   ├── ota-update-guide.md # Atualização OTA
│   ├── images/             # Diagramas, fotos, esquemas
│   └── datasheets/         # Datasheets dos componentes
├── examples/               # Exemplos didáticos numerados
├── gallery/                # Fotos e renders do projeto
├── .github/                # CI/CD e templates
├── AGENTS.md               # ESTE ARQUIVO
├── README.md               # Entry point principal
├── CONTRIBUTING.md         # Guia de contribuição
├── CHANGELOG.md            # Histórico de versões
├── LICENSE                 # GPL-3.0 + CERN-OHL-S-2.0 + CC-BY-SA-4.0
└── .gitignore
```

## Convenções de Código

### Firmware (C/C++ Arduino + PlatformIO)

- **Framework Arduino** com PlatformIO como build system
- **Pinos centralizados** em `firmware/include/pins.h` — nunca hardcodar GPIOs
- **Constantes** em `firmware/include/config.h`
- **Um módulo por par** (.h + .cpp) em `include/` e `src/`
- **ISR** sempre com `ICACHE_RAM_ATTR`
- **Variáveis de ISR** sempre `volatile`
- **Seções críticas** com `noInterrupts()` / `interrupts()`
- Credenciais via `.env` (build_flags no platformio.ini) — **NUNCA commitar `.env`**
- Código deve ser didático e legível para alunos de graduação

### Documentação

- Markdown (`.md`) para todos os textos
- Imagens em `docs/images/`
- Manter README de cada subpasta atualizado

### Hardware

- KiCad para PCBs (`.kicad_*` em `hardware/kicad/`)
- FreeCAD para peças 3D (`.FCStd` em `mechanical/cad/`)
- Gerbers empacotados em `.zip` com versão em `hardware/gerbers/`
- BOM em CSV em `hardware/bom/`

## Mapeamento de Pinos (Referência Rápida)

| GPIO | Pino NodeMCU | Função | Direção |
|---|---|---|---|
| GPIO5 | D1 | Encoder Canal A (LPD3806) | Input |
| GPIO4 | D2 | Encoder Canal B (LPD3806) | Input |
| GPIO14 | D5 | Motor Direção IN3 | Output |
| GPIO12 | D6 | Motor Direção IN4 | Output |
| GPIO13 | D7 | Motor PWM ENB | Output |
| GPIO2 | LED | LED onboard (ativo-baixo) | Output |

## Licenças

| Tipo | Licença |
|---|---|
| Firmware (código) | GPL-3.0 |
| Hardware (PCB, esquemático) | CERN-OHL-S-2.0 |
| Documentação e imagens | CC-BY-SA-4.0 |

## Antes de Commitar

1. **Firmware:** executar `cd firmware && pio run` — garantir que compila sem erros
2. **Segurança:** verificar que `.env`, credenciais ou secrets não estão sendo commitados
3. **Mensagens de commit:** usar formato convencional (ex: `feat:`, `fix:`, `docs:`)
4. **Atualizar CHANGELOG.md** se a mudança for significativa
5. **Atualizar READMEs** das pastas afetadas

## Dependências do Firmware

| Biblioteca | Versão | Uso |
|---|---|---|
| ElegantOTA | ^3.1.5 | Interface web para atualização OTA |
| ESP8266WebServer | ^1.0 | Servidor HTTP embarcado |

## API REST do Firmware

| Endpoint | Método | Parâmetros | Descrição |
|---|---|---|---|
| `/` | GET | — | Interface web de controle |
| `/api/motor` | POST | `direction` (forward/reverse/stop), `speed` (0-100) | Comando do motor (cancela PID/posição/auto-tune) |
| `/api/encoder` | GET | — | Retorna `pulses`, `rpm` e `angle` (graus) |
| `/api/status` | GET | — | Versão, heap, uptime, IP, clientes do AP e `mode` |
| `/api/pid/config` | GET/POST | `kp`, `ki`, `kd`, `setpoint` (RPM) | PID de velocidade |
| `/api/pid/start` `/api/pid/stop` | POST | — | Liga/desliga malha fechada de velocidade |
| `/api/pid/autotune` | GET/POST | `plant` (speed/position), `relay_amplitude`, `bias` (só speed), `cycles`, `setpoint` (RPM) ou `target` (graus) | Auto-tune relay feedback — bias vence a zona morta (speed); relé ±amplitude em torno do alvo (position) |
| `/api/pid/tuning` | GET | — | Ku, Tu e sugestões `zn`/`tl`/`cc` |
| `/api/pid/tuning/apply` | POST | `method` (ZN/TL/CC) | Aplica regra de sintonia |
| `/api/pid/response` | GET | — | Resposta ao degrau (tempo, setpoint, medida, `unit` rpm/deg) |
| `/api/position/config` | GET/POST | `kp`, `ki`, `kd`, `target` (graus) | PID de posição (ângulo) |
| `/api/position/start` | POST | `target` (opcional, graus) | Inicia controle de posição |
| `/api/position/stop` | POST | — | Para controle de posição |
| `/api/position/zero` | POST | — | Define a posição atual como 0° |
| `/update` | GET/POST | — | Interface OTA (ElegantOTA) |

## Notas Importantes

- O ESP8266 tem limitações de GPIO — nem todos os pinos suportam interrupção
- Para encoder em alta velocidade, considerar que o ESP8266 pode perder interrupções
- O sinal do encoder LPD3806-600BM opera em 5V; o ESP8266 é 3.3V — pode ser necessário Schmitt trigger (CD40106) para condicionamento de sinal
- O firmware tem **console serial** (115200): `help`, `status`, `enc`, `motor`, `pid`, `pos`, `autotune`, `tune` — permite controlar a bancada sem Wi-Fi
- **Sentido motor/encoder é auto-calibrado** no primeiro movimento (pulso de teste): fiação invertida é corrigida em software; velocidade/auto-tune usam |RPM| e são imunes a polaridade
- Bancada medida: zona morta do motor ~60% de duty (limita o controle de posição, que fica bang-bang) e 100% ≈ 1950 RPM; o relay do auto-tune usa bias 82% ± 18%
- Curva real duty×velocidade medida: 65% ≈ 5 RPM (atrito estático, movimento errático), 70% ≈ 400 RPM (região confiável), 100% ≈ 2000+ RPM — a malha de posição usa duty 70% (`POS_MAX_DUTY`)
- **Correia com folga**: erro residual de posição de ~1-2° é mecânico (backlash da correia), não do PID; deadband ±2° cobre
- Pulso de calibração de sentido amaciado: 70% × 50ms + freio (`MOTOR_CALIB_*`) — antes girava 400°+ (mais de uma volta) e parecia overshoot no gráfico
- Driver: **L298N** (queda de ~2 V é normal)
- OTA requer que o firmware atual + novo caibam na flash simultaneamente
- A linguagem do projeto é **português** para documentação e exemplos; **inglês** para código

## Contexto de Pesquisa e Publicação

O Girino visa gerar publicação acadêmica. Levantamento bibliográfico completo em `docs/referencias-pesquisa.md` (~70 refs) e análise comparativa em `docs/analise-comparativa.md`.

### Diferencial do Girino (estado da arte)

Plataformas existentes (MotoShield, OpenMCT, TCLab, Reck 2015) usam Arduino UNO/Teensy + MATLAB/Python. **Nenhuma oferece Wi-Fi nativo, interface web embarcada, API REST ou OTA.** O Girino propõe que o aluno interaja via navegador — sem instalar MATLAB, Python ou qualquer software.

### Trabalhos mais próximos

| Trabalho | Ano | Veículo | Diferença-chave |
|---|---|---|---|
| Cabral et al. — Mini bancada motor+encoder | 2025 | Ciência e Natura | Mesmo conceito, mas usa MATLAB/Simulink |
| Alexandre et al. — PID motor CC | 2025 | RET/UEPG | HW quase idêntico, mas sem Wi-Fi/web/OTA |
| MotoShield (Takács) | 2021 | IEEE EDUCON | Shield Arduino, sem conectividade |
| OpenMCT (Von Chong) | 2026 | HardwareX | Teensy 4 + GUI Python, sem web |
| Reck & Sreenivas | 2015/16 | ACC/MDPI | Papers fundacionais, sem Wi-Fi |

### Veículos recomendados para publicação

1. **COBENGE** (Congresso Brasileiro de Educação em Engenharia) — curto prazo
2. **CBA** (Congresso Brasileiro de Automática) — já publicou trabalhos similares
3. **RBEF** (Revista Brasileira de Ensino de Física) — lacuna clara, nenhum paper sobre motor+encoder+PID
4. **RBEE** (Revista Brasileira de Educação em Engenharia) — artigo completo com avaliação pedagógica

### Pendências para o artigo

- Implementar controle PID no firmware (ver `docs/plano-pid.md`)
- Implementar auto-tune via relay feedback (Åström-Hägglund)
- Comparar 3 métodos de sintonia: Ziegler-Nichols, Tyreus-Luyben, Cohen-Coon
- Modelagem matemática do sistema motor+encoder
- Interface web com gráficos de resposta ao degrau (Chart.js)
- Avaliação pedagógica com alunos
- Comparação experimental: simulação vs hardware real
