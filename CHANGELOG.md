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
- Interface web redesenhada em **3 abas** (Velocidade / Posição / Manual): o Auto-Tune virou um sub-card dentro da aba Velocidade (fluxo direto identificar → sintonizar → testar), com leitura ao vivo **Setpoint / Medido / Erro / Saída PWM** em destaque, botão **PARAR** fixo no cabeçalho (segurança), tooltips didáticos em Kp/Ki/Kd e box colapsável "Como funciona o PID?"; layout responsivo em 2 colunas em telas ≥ 900px
- **Auto-Tune de posição** (relé simétrico ±d em torno do alvo em graus): identifica a malha de ângulo e gera ganhos em %/° (Ku = 4d/πa) direto para o PID de posição; sub-card próprio na aba Posição, timeout de 90 s (ciclos de posição são mais lentos) e leitura ao vivo do relay ± durante o experimento

### Alterado
- Pinagem de referencia alinhada para motor com `IN3`/`IN4` e `ENB`:
  - `GPIO14 (D5)` -> `IN3`
  - `GPIO12 (D6)` -> `IN4`
  - `GPIO13 (D7)` -> `ENB/PWM`
- Documentacao atualizada com a nova referencia de pinagem em `AGENTS.md`, `firmware/README.md`, `docs/getting-started.md` e `docs/hardware-assembly.md`
- Interface web (`app.js`) conectada ao firmware real (`USE_MOCK=false`); simulador mantido para testes sem hardware
- Leitura do encoder usa `INPUT` confiando nos pull-ups externos de 4,7k para 3.3V (saída coletor aberto do LPD3806)
- Comando manual via `/api/motor` desliga automaticamente PID e auto-tune
- Interface: botão "Aplicar ganhos" removido — os ganhos/setpoint/alvo são enviados ao firmware automaticamente (debounce de 600 ms) ao digitar, ao escolher um método de sintonia (ZN/TL/CC) nos resultados do Auto-Tune e antes do "Iniciar"; mudar o setpoint durante a execução ajusta o alvo ao vivo
- PID refatorado em núcleo único compartilhado entre velocidade (0..+100%) e posição (-100..+100%)
- `/api/pid/tuning` responde chaves minúsculas (`zn`/`tl`/`cc`), consistente com a interface
- Trocar de modo na interface cancela o experimento em andamento no firmware
- Setpoint de velocidade limitado a 3000 RPM (antes 1000) e defaults ajustados à bancada medida (zona morta ~60%, 100% ≈ 1950 RPM): Auto-Tune com amplitude 18% e bias 82% (relay 64–100%), setpoint padrão 1000 RPM, campo de setpoint na UI até 2400 RPM
- PWM do motor para 4 kHz com 8 bits (255 passos) — chiado do chaveamento mais discreto, dentro do orçamento ~1 MHz do PWM por software do ESP8266
- `/api/pid/config` e `/api/position/config` (GET) retornam agora `running`, `error` e `output` (e `measurement` no PID de velocidade); `/api/pid/autotune` (GET) inclui o `output` do relé — alimenta a leitura ao vivo da interface (endpoints de POST inalterados)
- `/api/pid/autotune` (POST) aceita o parâmetro `plant` (`speed` padrão | `position`): em posição usa `target` (graus) como centro da oscilação, relé ±amplitude e ignora `bias`; `autotuneStart()` assinatura nova `(plant, amp, bias, ciclos, setpoint)`; `/api/pid/tuning` e `/api/pid/tuning/apply` aplicam os ganhos no controlador da planta identificada; console serial ganhou `autotune pos <amp> <n> <graus>`

### Corrigido
- Simulador da interface (`USE_MOCK`): o laço de controle agora roda a 50 Hz (5 sub-passos de 20 ms por tick do gráfico, igual ao firmware) — antes simulava a 10 Hz e os ganhos do auto-tune oscilavam em ciclo-limite sustentado na simulação (na bancada convergem); o auto-tune do mock mede o envelope real da oscilação (como o firmware) em vez de usar amplitude estimada fixa
- Planta do mock fiel à bancada real: zona morta ~60% de duty (motor parado abaixo dela, roda-livre como o STOP do L298N), 100% ≈ 1950 RPM, acionamento mínimo ~230 RPM — malha aberta, PID de velocidade, PID de posição e os dois auto-tunes compartilham a mesma planta; o PID de posição com ela reproduz o creep/bang-bang da bancada (documentado na dica da aba Posição)
- Freio dinâmico no L298N para a malha de posição: `MOTOR_DIR_BRAKE` (IN3+IN4 HIGH) + `motorBrake()`; quando a saída do PID de posição está abaixo da zona morta do motor (`POS_MOTOR_DEADZONE_DUTY 60`), o firmware freia o eixo em vez de rodar-livre — mata a inércia do coast que causava o overshoot de cada chute do bang-bang (na simulação: de 1% para 99% do tempo dentro de ±2° do alvo); UI: tile ERRO da posição fica verde quando o erro está dentro da deadband (±2°)
- Posição: atuação em **passos discretos** perto do alvo (modo passo em 3 camadas): deadband/freio → passos curtos a `POS_SOFT_DUTY 65%` com direção pelo sinal do erro e pausa de freio entre passos (`POS_SOFT_KICK_TICKS 1` + `POS_SOFT_REST_TICKS 3` a 50 Hz) → longe, duty mínimo `POS_MIN_DRIVE_DUTY 62` acima da zona morta (pedidos fracos não travam no limiar). Correções no núcleo PID: teto do termo integral da posição (`POS_INTEGRAL_TERM_MAX 55`, abaixo da zona morta — sem rastejo com erro ~0) e teto da contribuição derivativa (`POS_DERIVATIVE_TERM_MAX 30`) — o D a toda velocidade inverteria a saída a cada tick e era a causa principal da caça ±35° medida na bancada; validação numérica: 100% do tempo na faixa ±2° com ganhos default e com ganhos do auto-tune (antes: 0%)
- **Posição v2 (validada em bancada, 9/9)**: o ciclo soco-100%/freio a 50 Hz levou o L298N ao **desligamento térmico** (eixo travou a 171° além do alvo) — duty da malha de posição limitado a `POS_MAX_DUTY 65` (nunca 100%), passos de 2 ticks + descanso de freio de 8 ticks (160 ms) para o driver esfriar; bancada: assentou a **1.8° do alvo com Out=0% e eixo travado**, estável; a caça ±35° de antes era o D explodindo + o driver desligando
- Interface: gráfico movido para o **topo da página** (largura total, abaixo das abas); abaixo dele o painel do modo ativo + status em 2 colunas (desktop) ou empilhados (mobile)
- **Otimização de overshoot validada na bancada (teste agressivo)**: (1) pulso de calibração de sentido amaciado — 70% × 50 ms + freio (`MOTOR_CALIB_*`): giro caiu de **416° para 32-104°** (antes parecia overshoot no gráfico logo ao iniciar); (2) posição ganhou **zona de frenagem antecipada** (`POS_BRAKE_ZONE_*`: com erro < 60° e eixo em movimento, freia antes de entrar nos passos) e duty ajustado para **70%** (`POS_MAX_DUTY` — a curva real medida mostra 65% ≈ 5 RPM instável por atrito estático e 70% ≈ 400 RPM confiável): degrau 0→180° caiu de **48.6° para 4.8°** de overshoot e de 18.5 s para 3.4 s de acomodação; com ganhos agressivos (Kp=5) o overshoot caiu de 29° para ~0°; (3) Kd default do PID de velocidade 0 → **0.005** (overshoot do degrau 500→1500 RPM: 6.7% → 3.7%)
- Documentado na UI e no `AGENTS.md`: **folga mecânica da correia** — o erro residual de posição de ~1-2° é físico (backlash), não do PID; a deadband de ±2° cobre
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
