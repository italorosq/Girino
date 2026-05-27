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
| `cd firmware && pio run -t upload` | Compilar e gravar via USB |
| `cd firmware && pio run -t upload --upload-port girino.local` | Gravar via OTA |
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
| GPIO14 | D5 | Motor PWM | Output |
| GPIO12 | D6 | Motor Direção IN1 | Output |
| GPIO13 | D7 | Motor Direção IN2 | Output |
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
| `/api/motor` | POST | `direction` (forward/reverse/stop), `speed` (0-100) | Comando do motor |
| `/api/encoder` | GET | — | Retorna `pulses` e `rpm` |
| `/api/status` | GET | — | Versão, heap livre, uptime, RSSI WiFi |
| `/update` | GET/POST | — | Interface OTA (ElegantOTA) |

## Notas Importantes

- O ESP8266 tem limitações de GPIO — nem todos os pinos suportam interrupção
- Para encoder em alta velocidade, considerar que o ESP8266 pode perder interrupções
- O sinal do encoder LPD3806-600BM opera em 5V; o ESP8266 é 3.3V — pode ser necessário Schmitt trigger (CD40106) para condicionamento de sinal
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
