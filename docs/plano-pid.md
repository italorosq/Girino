# Plano — Módulo PID e Auto-Tune

Planejamento do controle PID com auto-tune e comparação de métodos de sintonia para o artigo.

---

## 1. Métodos de Sintonia a Implementar

### Por que comparar métodos?

Todos os trabalhos relacionados usam **um único método** de sintonia, dependendo de MATLAB ou Python. Nenhum oferece comparação visual entre métodos diretamente no hardware. O Girino propõe que o aluno compare ZN, TL e CC no navegador — vendo diferenças de overshoot, tempo de acomodação e erro em regime.

### Métodos selecionados

| # | Método | Tipo | Referência | Por que incluir |
|---|--------|------|------------|-----------------|
| 1 | **Ziegler-Nichols** | Regra (após relay) | Ziegler & Nichols, 1942 | Mais conhecido. Referência obrigatória em qualquer texto de controle. |
| 2 | **Tyreus-Luyben** | Regra (após relay) | Tyreus & Luyben, 1992 | Mais robusto que ZN, menor overshoot. Bom contraste didático. |
| 3 | **Cohen-Coon** | Regra (após relay) | Cohen & Coon, 1953 | Usado por Alexandre (2025) — permite comparação direta com literatura. |
| 4 | **Relay Feedback** | Auto-tune | Åström & Hägglund, 1984 | Método de identificação. Encontra Ku e Tu automaticamente. **Diferencial do Girino.** |
| 5 | **Manual** | Livre | — | Professor/aluno ajusta Kp, Ki, Kd via web. Maior valor didático. |

### Coeficientes de cada regra (a partir de Ku, Tu)

| Regra | Kp | Ti | Td |
|---|---|---|---|
| **Ziegler-Nichols** | 0.6 × Ku | Tu / 2.0 | Tu / 8.0 |
| **Tyreus-Luyben** | Ku / 2.2 | 2.2 × Tu | Tu / 6.3 |
| **Cohen-Coon** | (Ku/1.35) × (1 + Tu/(4×delay)) | Tu × f(delay) | Tu × g(delay) |

Nota: Cohen-Coon requer estimativa de atraso (dead time) — pode ser obtido via step test.

---

## 2. Fluxo de Auto-Tune (Relay Feedback)

```
Aluno clica "Auto-Tune" na interface web
       ↓
[1] Motor em velocidade nominal (setpoint inicial)
       ↓
[2] ESP8266 aplica relay: +d% se erro > 0, -d% se erro < 0
    (d = amplitude do relay, ex: 20% do PWM)
       ↓
[3] Sistema oscila em limite cycle
    Encoder mede período da oscilação → Tu (período crítico)
    Amplitude do relay × ganho do sensor → Ku (ganho crítico)
       ↓
[4] Calcula Ku e Tu pela média de N ciclos (N=3 a 5)
       ↓
[5] Oferece 3 opções de ganhos: ZN, TL, CC
    Aluno escolhe ou testa todas
       ↓
[6] PID carregado com Kp, Ki, Kd calculados
       ↓
[7] Resposta ao degrau registrada e exibida no gráfico web
       ↓
[8] Aluno pode comparar overshoot, settling time, steady-state error
```

### Parâmetros do Relay Feedback

| Parâmetro | Valor | Observação |
|---|---|---|
| Amplitude do relay (d) | 15–25% PWM | Ajustável. Motor não pode parar. |
| Número de ciclos (N) | 3–5 | Mais ciclos = mais preciso, mais demorado |
| Setpoint nominal | 50% velocidade | Ponto de operação para o experimento |
| Timeout | 30 segundos | Segurança — aborta se não oscilar |

---

## 3. Arquitetura do Firmware

### Novos arquivos

```
firmware/include/
├── pid.h                  → Estrutura PID (Kp, Ki, Kd, setpoint, limites)
├── pid_autotune.h         → Relay feedback (Ku, Tu, estado do experimento)
└── pid_tuning_rules.h     → Calcula Kp/Ki/Kd para ZN, TL, CC

firmware/src/
├── pid.cpp                → Cálculo PID com anti-windup + derivative-on-measurement
├── pid_autotune.cpp       → Máquina de estados do relay feedback
└── pid_tuning_rules.cpp   → Aplica regras de sintonia
```

### PID — Formulacao

```
Implementar forma posicional (standard):

u(t) = Kp × e(t) + Ki × ∫e(τ)dτ + Kd × de(t)/dt

Discreto (T = período de amostragem):

u[k] = Kp × e[k]
     + Ki × T × Σe[j]     (com anti-windup: clamp na integral)
     + Kd × (e[k] - e[k-1]) / T

Ou, para evitar derivative kick:
u[k] = Kp × e[k]
     + Ki × T × Σe[j]
     + Kd × (y[k] - y[k-1]) / T     (derivative on measurement)

Saída u[k] mapeada para PWM: 0 a MOTOR_PWM_RANGE (1023)
```

### Anti-windup

A integral do PID deve ser limitada para evitar saturação:
- Se `u[k] > max_output` → clamp integral em `max_output - Kp×e - Kd×de`
- Se `u[k] < min_output` → clamp integral similarmente

### Sample rate

| Taxa | Observação |
|---|---|
| 50 Hz (20 ms) | Boa para motor DC. ESP8266 aguenta facilmente. |
| 100 Hz (10 ms) | Se encoder de alta velocidade exigir. |
| 10 Hz (100 ms) | Taxa atual do encoder. Pode ser suficiente. |

**Recomendação:** 50 Hz (20ms). Encoder já amostra a 100ms — pode diminuir para 20ms no modo PID.

---

## 4. API REST — Novos Endpoints

| Endpoint | Método | Parâmetros | Descrição |
|---|---|---|---|
| `/api/pid/config` | POST | `kp`, `ki`, `kd`, `setpoint` | Configura ganhos e setpoint do PID |
| `/api/pid/config` | GET | — | Retorna config atual |
| `/api/pid/start` | POST | — | Liga controle em malha fechada |
| `/api/pid/stop` | POST | — | Desliga PID, volta a malha aberta |
| `/api/pid/autotune` | POST | `relay_amplitude`, `cycles` | Inicia experimento relay feedback |
| `/api/pid/autotune` | GET | — | Status do auto-tune (running/done/results) |
| `/api/pid/tuning` | GET | — | Retorna Ku, Tu + sugestões ZN/TL/CC |
| `/api/pid/tuning/apply` | POST | `method` (ZN/TL/CC) | Aplica regra de sintonia escolhida |
| `/api/pid/response` | GET | — | Última resposta ao degrau (array tempo, setpoint, velocidade) |

### Exemplo de uso

```bash
# 1. Iniciar auto-tune
curl -X POST http://192.168.1.100/api/pid/autotune \
  -d "relay_amplitude=20&cycles=3"

# 2. Ver resultados
curl http://192.168.1.100/api/pid/autotune
# → {"status":"done","ku":5.2,"tu":0.35}

# 3. Ver sugestões
curl http://192.168.1.100/api/pid/tuning
# → {"ku":5.2,"tu":0.35,
#     "ZN":{"kp":3.12,"ki":17.83,"kd":0.14},
#     "TL":{"kp":2.36,"ki":4.55,"kd":0.06},
#     "CC":{"kp":3.85,"ki":15.20,"kd":0.12}}

# 4. Aplicar Tyreus-Luyben
curl -X POST http://192.168.1.100/api/pid/tuning/apply \
  -d "method=TL"

# 5. Iniciar malha fechada
curl -X POST http://192.168.1.100/api/pid/start \
  -d "setpoint=50"

# 6. Coletar resposta ao degrau
curl http://192.168.1.100/api/pid/response
# → {"data":[[0,0],[0.02,5],...,[3.0,49.8]],"setpoint":50}
```

---

## 5. Interface Web — Componentes Novos

### Painel de PID (adicionar à página principal)

```
┌─────────────────────────────────────┐
│  Controle PID                       │
│                                     │
│  Setpoint: [____] RPM               │
│  Kp: [____]  Ki: [____]  Kd: [____] │
│                                     │
│  [▶ Iniciar PID]  [⏹ Parar]         │
│  [🔧 Auto-Tune]  [📊 Step Response] │
│                                     │
│  Método de sintonia:                │
│  ○ Ziegler-Nichols                  │
│  ○ Tyreus-Luyben (recomendado)      │
│  ○ Cohen-Coon                       │
│  ○ Manual                           │
│                                     │
│  ┌─────────────────────────────┐    │
│  │  Gráfico: Tempo vs RPM      │    │
│  │  (setpoint + resposta real)  │    │
│  │  ~~~~~~~~~~~~~~~~~~~~~~~~    │    │
│  └─────────────────────────────┘    │
│                                     │
│  Resultados Auto-Tune:              │
│  Ku = 5.2  Tu = 0.35s              │
│  Overshoot: 12%  Settling: 1.8s     │
└─────────────────────────────────────┘
```

### Biblioteca de gráficos

Usar **Chart.js** (CDN, leve, sem instalação):
- Gráfico de linha em tempo real
- Eixo X: tempo (s), Eixo Y: RPM
- Linha tracejada: setpoint
- Linha sólida: velocidade real
- Exportar dados como CSV

---

## 6. Estrutura Didática (Exemplos Progressivos)

| # | Exemplo | Conceito | Atividade do Aluno |
|---|---------|----------|--------------------|
| 01 | Controle básico de motor | PWM, duty cycle | Variar PWM e observar velocidade |
| 02 | Leitura de encoder | Interrupção, quadratura | Contar pulsos, calcular RPM |
| 03 | Curva do motor (malha aberta) | Modelagem empírica | Aplicar PWM de 0 a 100%, plotar curva RPM vs PWM |
| 04 | PID manual | Controle em malha fechada | Ajustar Kp, Ki, Kd e observar resposta |
| 05 | Auto-tune | Identificação de sistemas | Executar relay feedback, obter Ku e Tu |
| 06 | Comparação de métodos | ZN vs TL vs CC | Aplicar cada método, comparar gráficos |

### Valor didático da comparação de métodos

O aluno aprende que:
- ZN tem resposta rápida mas muito overshoot
- TL é mais lento mas sem oscilação
- CC é intermediário
- Manual pode ser melhor se o aluno entende o sistema
- Não existe "melhor método" — depende do critério (velocidade, robustez, etc.)

---

## 7. Comparação com Trabalhos Existentes

| Aspecto | Alexandre 2025 | Cabral 2025 | MotoShield | OpenMCT | **Girino** |
|---|---|---|---|---|---|
| Método de sintonia | Cohen-Coon | z-domain (MATLAB) | MATLAB sysID | Coeficientes GUI | **Auto-tune + 3 regras + manual** |
| Auto-tune embarcado | ❌ | ❌ | ❌ | ❌ | **✅ (Relay Feedback)** |
| Comparação de métodos | ❌ | ❌ | ❌ | ❌ | **✅ (ZN vs TL vs CC)** |
| Visualização | SCADA externo | MATLAB | MATLAB | Python/Qt | **Navegador web** |
| Software necessário | SCADA | MATLAB | MATLAB | Python+Qt | **Nenhum** |

### Argumento para o paper

> "Enquanto plataformas existentes usam um único método de sintonia dependente de software externo (MATLAB, Python), o Girino implementa auto-tune embarcado via relay feedback (Åström-Hägglund) e permite comparação direta de três regras clássicas (Ziegler-Nichols, Tyreus-Luyben, Cohen-Coon) através de interface web — sem instalação de qualquer software."

---

## 8. Pendências para o Artigo (Checklist)

- [x] Levantamento bibliográfico (~70 referências)
- [x] Análise comparativa dos trabalhos relacionados
- [x] Planejamento dos métodos de sintonia
- [ ] Implementar PID no firmware (pid.h + pid.cpp)
- [ ] Implementar auto-tune (pid_autotune.h + pid.cpp)
- [ ] Implementar regras de sintonia (ZN, TL, CC)
- [ ] Adicionar endpoints API REST para PID
- [ ] Interface web com gráfico de resposta ao degrau
- [ ] Modelagem matemática do sistema motor+encoder
- [ ] Testes experimentais: comparar ZN vs TL vs CC
- [ ] Avaliação pedagógica com alunos
- [ ] Redação do artigo
