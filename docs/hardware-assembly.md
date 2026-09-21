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
         Motor IN3  │              │ Motor IN4
                    │              │
              GPIO13│              │ GPIO2
         (D7) ────→│              │←──── (D4)
         Motor ENB  │              │ LED onboard
            PWM     │              │
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
| Canal A | Amarelo | GPIO5 (D1) + pull-up 4,7k para 3.3V |
| Canal B | Verde | GPIO4 (D2) + pull-up 4,7k para 3.3V |

#### ⚠️ Importante: saída em coletor aberto

O LPD3806-600BM tem **saída em coletor aberto** (open-collector): cada canal
não "empurra" o sinal para alto — apenas **puxa o pino para GND** quando o
transistor interno conduz. Sem um resistor de pull-up, o pino do ESP8266
fica **flutuando** e a leitura não funciona.

```
        VCC (5V) ──── LED interno do encoder
        VCC (5V) ──── coletor do transistor (via encoder)
                          │
                      saída A/B ──→ pino do ESP8266
                          │
                     GND (quando conduz)
```

Por isso **cada canal (A e B) precisa de um resistor de pull-up de 4,7k
(ou 10k) conectado ao 3.3V do ESP8266**:

```
  Saída A do encoder ──●────→ D1 (GPIO5)
                       │
                      [R] 4,7k
                       │
  3.3V do ESP8266 ─────┘   (idem para canal B em D2)
```

Regras de segurança:

1. **O pull-up vai para 3.3V, nunca para 5V.** Os GPIOs do ESP8266 não
   são tolerantes a 5V. Como a saída é coletor aberto, o transistor só
   conecta o pino ao GND — a tensão alta do sinal vem do pull-up, então
   mantendo o pull-up em 3.3V o sinal fica seguro.
2. **Por que não usar o pull-up interno do ESP8266?** Ele existe
   (`INPUT_PULLUP`), mas é fraco (~10k–50k, impreciso) e produz bordas
   de subida lentas — em RPM alto, a interrupção perde pulsos. O resistor
   externo de 4,7k dá bordas firmes e contagem confiável.
3. Os resistores já estão soldados na PCB do Girino (ver esquemático em
   `hardware/kicad/`). O firmware usa `pinMode(..., INPUT)` simples,
   confiando nos pull-ups externos.

### Driver do Motor

| Pino Driver | Conexão |
|---|---|
| INB1 (alias IN3) | GPIO14 (D5) |
| INB2 (alias IN4) | GPIO12 (D6) |
| ENB (PWM) | GPIO13 (D7) |
| VCC (Vm) | Fonte 12V do motor |
| VCC (Vcc) | 3.3V (lógica) |
| GND | GND comum |
| OUTB1 / OUTB2 | Motor DC |

Nota: os canais ENA/INA1/INA2/OUTA não são usados pela PCB e ficam
desconectados (ver nets do esquemático).

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
| Encoder não conta | Pinos errados ou pull-up ausente | Verificar GPIO5/GPIO4 e os resistores de pull-up 4,7k para 3.3V (coletor aberto exige pull-up externo) |
| Leitura do encoder instável / RPM errado | Pull-up fraco ou para 5V | Confirmar pull-up externo de 4,7k para **3.3V** (não 5V) |
| Motor não gira | Driver sem alimentação | Verificar VCC no driver |
| Motor gira errado | INB1/INB2 invertidos | Trocar conexões INB1 ↔ INB2 |
