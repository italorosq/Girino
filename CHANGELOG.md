# Changelog

Todas as mudanças notáveis neste projeto serão documentadas neste arquivo.

O formato é baseado em [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/),
e este projeto adota [Versionamento Semântico](https://semver.org/lang/pt-BR/).

## [Unreleased]

### Adicionado
- Exemplo `05-pid-web-test` com firmware standalone para teste PID via interface web
- Painel web no exemplo com ajuste em tempo real de `setpoint`, `Kp`, `Ki`, `Kd` e grafico de resposta
- Controle PID em malha fechada no firmware (`pid.cpp`, 50 Hz, anti-windup, derivative-on-measurement)
- Auto-tune embarcado via relay feedback Åström-Hägglund (`pid_autotune.cpp`): identifica Ku e Tu
- Regras de sintonia Ziegler-Nichols, Tyreus-Luyben e Cohen-Coon (`pid_tuning_rules.cpp`)
- Endpoints REST `/api/pid/config`, `/api/pid/start`, `/api/pid/stop`, `/api/pid/autotune`, `/api/pid/tuning`, `/api/pid/tuning/apply` e `/api/pid/response`
- Campo `mode` no `/api/status` (`open-loop`/`pid`/`autotune`) para comparação entre malha aberta e fechada
- Controle de posição (ângulo) com PID bidirecional: `/api/position/config`, `/api/position/start`, `/api/position/stop` e `/api/position/zero`
- Modo **Posição** na interface web (alvo em graus, ganhos próprios, botão "Zerar Posição", gráfico em graus)
- `angle` (graus) no `/api/encoder` e no `/api/status`; campo `mode` aceita `position`
- `stations`, `ssid` e `ip` no `/api/status` (rede do Access Point)
- Exportação CSV do experimento gravado no ESP (`/api/pid/response`), com unidade rpm/graus
- Chart.js embarcado no LittleFS (`data/chart.umd.min.js`) — a interface funciona sem internet
- `scripts/load_env.py`: credenciais do `.env` injetadas corretamente na compilação
- Girino agora opera como **Access Point** (`GIRINO_AP`, `http://192.168.4.1`)
- Feedback do encoder: LED onboard pisca a cada pulso, telemetria serial de 1 Hz (`pulsos`, `p/s`, `rpm`, `ângulo`) e tile "Encoder" na interface com alerta quando o motor roda sem sinal
- Telemetria serial 2 Hz dos controladores ativos: `[PID] SP/RPM/Out`, `[Pos] Alvo/Ângulo/Out` e `[AutoTune] RPM/relay`
- Ganhos default do PID de velocidade calibrados para a bancada (Kp=0.08, Ki=0.25, Kd=0) e aviso na interface quando os ganhos estão zerados
- Console serial de comandos (`help`, `status`, `enc`, `motor`, `pid`, `pos`, `autotune`, `tune`) — controle completo da bancada sem Wi-Fi
- Auto-calibração do sentido motor/encoder: no primeiro movimento após o boot um pulso de teste detecta fiação invertida e corrige em software — "horário" sempre aumenta o ângulo, qualquer que seja a ligação dos fios
- Interface: botões Iniciar/Parar dentro dos painéis de PID e Posição, descrição de cada modo, passo a passo do controle de posição e nota de que o Auto-Tune sugere ganhos para a malha de velocidade

### Alterado
- Pinagem de referencia alinhada para motor com `IN3`/`IN4` e `ENB`:
  - `GPIO14 (D5)` -> `IN3`
  - `GPIO12 (D6)` -> `IN4`
  - `GPIO13 (D7)` -> `ENB/PWM`
- Documentacao atualizada com a nova referencia de pinagem em `AGENTS.md`, `firmware/README.md`, `docs/getting-started.md` e `docs/hardware-assembly.md`
- Interface web (`app.js`) conectada ao firmware real (`USE_MOCK=false`); simulador mantido para testes sem hardware
- Leitura do encoder usa `INPUT` confiando nos pull-ups externos de 4,7k para 3.3V (saída coletor aberto do LPD3806)
- Comando manual via `/api/motor` desliga automaticamente PID e auto-tune
- PID refatorado em núcleo único compartilhado entre velocidade (0..+100%) e posição (-100..+100%)
- `/api/pid/tuning` responde chaves minúsculas (`zn`/`tl`/`cc`), consistente com a interface
- Trocar de modo na interface cancela o experimento em andamento no firmware
- Setpoint de velocidade limitado a 3000 RPM (antes 1000) e defaults ajustados à bancada medida (zona morta ~60%, 100% ≈ 1950 RPM): Auto-Tune com amplitude 18% e bias 82% (relay 64–100%), setpoint padrão 1000 RPM, campo de setpoint na UI até 2400 RPM
- PWM do motor para 4 kHz com 8 bits (255 passos) — chiado do chaveamento mais discreto, dentro do orçamento ~1 MHz do PWM por software do ESP8266

### Corrigido
- Senha do OTA não era aplicada (`ElegantOTA.begin` chamado sem usuário/senha corretos)
- Polaridade do encoder alinhada (horário/FORWARD = ângulo positivo) — pré-requisito do controle de posição
- Aquisição de RPM do laço de controle: janela deslizante de 80 ms + limitador de slew — elimina os picos causados por pulsos perdidos nas rajadas de Wi-Fi do ESP8266
- Malha de velocidade e auto-tune usam o módulo da velocidade (|RPM|) como realimentação — imunes à inversão de fiação; a posição continua usando o ângulo com sinal
- Auto-tune: debounce de cruzamentos (60 ms), envelope medido a partir do 2º cruzamento e período completo (entre cruzamentos pares) — Ku/Tu agora são válidos (antes Tu saía pela metade e a amplitude vinha poluída pelo transiente de partida)
- PWM de 4 kHz com 8 bits validado em bancada (motor gira normalmente, chiado do chaveamento mais discreto)
- Auto-Tune travava em motores com zona morta alta (só zumbia sem girar): relay agora alterna em torno de um pedestal `bias` (default 82%) e Ku usa a amplitude efetiva do degrau; campo Bias na UI e parâmetro `bias` na API
- Interface web travava em "conectando..." no modo AP por depender de Chart.js via CDN
- Auto-Tune: falha/timeout não deixava mais o botão travado; adicionado estado de erro e botão Parar funcional durante o experimento
- Seleção de método de sintonia (ZN/TL/CC) agora preenche os ganhos corretamente
- Capacidade de gravar a interface web (`data/`) com `pio run -t uploadfs`

## [0.1.0] - 2025-05-27

### Adicionado
- Estrutura inicial do repositório
- Firmware skeleton (PlatformIO + ESP8266)
  - Módulo de controle de motor (PWM)
  - Módulo de leitura de encoder (LPD3806-600BM)
  - Módulo OTA (ElegantOTA)
  - Interface web básica
- Templates de documentação
- Exemplos didáticos (placeholders)
- Licenças: GPL-3.0 (firmware), CERN-OHL-S-2.0 (hardware), CC-BY-SA-4.0 (docs)
- Guia de contribuição
