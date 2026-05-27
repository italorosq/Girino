# Exemplo 05 — Teste PID com Interface Web

Exemplo standalone para validar encoder, acionamento do motor e malha PID com ajuste em tempo real pelo navegador.

## Objetivos de Aprendizagem

- Verificar se a pinagem do hardware esta correta
- Ajustar ganhos `Kp`, `Ki`, `Kd` em tempo real
- Visualizar resposta de posicao vs setpoint no grafico

## Pinagem de Referencia (este exemplo)

| GPIO | Pino NodeMCU | Funcao |
|---|---|---|
| GPIO5 | D1 | Encoder Canal A |
| GPIO4 | D2 | Encoder Canal B |
| GPIO14 | D5 | Direcao IN3 |
| GPIO12 | D6 | Direcao IN4 |
| GPIO13 | D7 | PWM ENB |

## Arquivos

- `src/main.cpp` — Firmware de teste com servidor web + PID
- `platformio.ini` — Configuracao PlatformIO do exemplo

## Como executar

1. Edite `src/main.cpp` e ajuste `ssid` e `password`
2. Compile e grave no ESP8266:

```bash
cd examples/05-pid-web-test
pio run -t upload
pio device monitor
```

3. No monitor serial, copie o IP exibido e abra no navegador:

```text
http://<IP_DO_ESP8266>/
```

## Endpoints

| Endpoint | Metodo | Descricao |
|---|---|---|
| `/` | GET | Painel web com ajustes de PID e grafico |
| `/status` | GET | Retorna `posicao` e `setpoint` em JSON |
| `/set` | GET | Atualiza `sp`, `kp`, `ki`, `kd` |

## Checklist de teste rapido

1. Com `setpoint = 0`, confirme motor parado
2. Ajuste `setpoint` para um valor pequeno e confirme movimento
3. Inverta o sinal do `setpoint` e confirme inversao de sentido
4. Verifique se o grafico acompanha `setpoint` e `posicao`
