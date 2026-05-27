# Exemplo 03 — Controle PID de Velocidade

Implementação de controlador PID para manter velocidade constante do motor.

## Objetivos de Aprendizagem

- Entender o conceito de controle em malha fechada
- Implementar um controlador PID
- Sintonizar os ganhos Kp, Ki, Kd

## Conceitos

### Malha Fechada

```
  Setpoint ──→ [+] ──→ PID ──→ Motor ──→ Velocidade
                ↑ -                         │
                └──── Encoder (feedback) ───┘
```

### PID (Proporcional-Integral-Derivativo)

- **P (Proporcional):** Erro × Kp → reação proporcional ao erro
- **I (Integral):** Soma do erro × Ki → elimina erro em regime permanente
- **D (Derivativo):** Taxa de variação × Kd → amortecimento de oscilações

## Código

> Em breve — será adicionado com a implementação.

## Exercícios Propostos

1. Implementar apenas o P e observar o erro em regime
2. Adicionar I e eliminar o erro em regime permanente
3. Adicionar D e reduzir overshoot
4. Aplicar o método Ziegler-Nichols para sintonia
5. Testar resposta a perturbações (carga variável no motor)
