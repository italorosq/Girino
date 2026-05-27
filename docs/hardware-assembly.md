# Montagem do Hardware — Girino

## Lista de Materiais (BOM)

Ver lista completa em [`hardware/bom/`](../hardware/bom/).

## Ferramentas Necessárias

- Ferro de solda e solda
- Multímetro
- Alicate de corte e descascador de fios
- Chaves Philips e Allen

## Diagrama de Ligação

### ESP8266 WROOM — Pinagem

```
                    ┌──────────────┐
         (TX) GPIO1 │              │ GPIO3 (RX)
              GPIO5 │              │ GPIO4
         (D1) ←────│  ESP8266     │────→ (D2)
         Encoder A  │  WROOM      │ Encoder B
                    │              │
              GPIO14│              │ GPIO12
         (D5) ────→│              │←──── (D6)
         Motor PWM  │              │ Motor DIR IN1
                    │              │
              GPIO13│              │ GPIO2
         (D7) ────→│              │←──── (D4)
         Motor DIR  │              │ LED onboard
           IN2      │              │
                    │              │
              3V3   │              │  GND
              3.3V  │              │  GND
                    └──────────────┘
```

### Encoder LPD3806-600BM

| Fio | Cor (típica) | Conexão |
|---|---|---|
| VCC | Vermelho | 5V |
| GND | Preto | GND |
| Canal A | Amarelo | GPIO5 (D1) |
| Canal B | Verde | GPIO4 (D2) |

### Driver do Motor

| Pino Driver | Conexão |
|---|---|
| IN1 | GPIO12 (D6) |
| IN2 | GPIO13 (D7) |
| PWM / EN | GPIO14 (D5) |
| VCC | Fonte do motor |
| GND | GND comum |
| OUT1 / OUT2 | Motor DC |

## Passo a Passo

### 1. Preparação da Fonte

1. Conectar o módulo de conversão de tensão à fonte principal
2. Ajustar a saída para 5V (para o ESP8266 e encoder)
3. Verificar tensão com multímetro

### 2. Montagem do Encoder

1. Fixar o encoder no eixo do motor
2. Conectar os fios conforme tabela acima
3. Verificar que os canais A e B estão em GPIOs com interrupção

### 3. Conexão do Driver do Motor

1. Conectar os pinos de controle ao ESP8266
2. Conectar a saída do driver ao motor DC
3. Alimentar o driver com tensão adequada ao motor

### 4. Conexão do ESP8266

1. Alimentar o ESP8266 com 3.3V (via regulador ou USB)
2. Conectar todos os GNDs em comum
3. Verificar todas as conexões com multímetro antes de ligar

## Verificação

Antes de prosseguir para o firmware:

1. ✅ Todas as conexões GND em comum
2. ✅ Tensões corretas em cada ponto
3. ✅ Nenhum curto-circuito entre VCC e GND
4. ✅ Encoder girando livremente com o motor
5. ✅ ESP8266 ligando (LED onboard piscando)

## Solução de Problemas

| Problema | Causa Provável | Solução |
|---|---|---|
| ESP8266 não liga | Alimentação inadequada | Verificar 3.3V no pino |
| Encoder não conta | Pinos errados ou sem pull-up | Verificar GPIO5/GPIO4 |
| Motor não gira | Driver sem alimentação | Verificar VCC no driver |
| Motor gira errado | IN1/IN2 invertidos | Trocar conexões IN1 ↔ IN2 |
