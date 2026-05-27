# Changelog

Todas as mudanças notáveis neste projeto serão documentadas neste arquivo.

O formato é baseado em [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/),
e este projeto adota [Versionamento Semântico](https://semver.org/lang/pt-BR/).

## [Unreleased]

### Adicionado
- Exemplo `05-pid-web-test` com firmware standalone para teste PID via interface web
- Painel web no exemplo com ajuste em tempo real de `setpoint`, `Kp`, `Ki`, `Kd` e grafico de resposta

### Alterado
- Pinagem de referencia alinhada para motor com `IN3`/`IN4` e `ENB`:
  - `GPIO14 (D5)` -> `IN3`
  - `GPIO12 (D6)` -> `IN4`
  - `GPIO13 (D7)` -> `ENB/PWM`
- Documentacao atualizada com a nova referencia de pinagem em `AGENTS.md`, `firmware/README.md`, `docs/getting-started.md` e `docs/hardware-assembly.md`

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
