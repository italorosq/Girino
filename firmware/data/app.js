/**
 * Girino — Interface Web (app.js)
 *
 * Módulos:
 *   Simulator   — modelo mock do motor+encoder+PID para testes sem hardware
 *   APIClient   — cliente HTTP para os endpoints /api/*
 *   ChartManager — gerencia Chart.js com rolling window
 *   UIController — lógica da interface, event listeners, polling
 *
 * Interface por ABAS: Velocidade (PID + Auto-Tune) | Posição | Manual
 * Leitura ao vivo: setpoint, medida, erro e saída — o conceito central do PID.
 */

// ============================================================
// CONFIGURAÇÃO
// ============================================================

const CONFIG = {
  USE_MOCK: false,            // true = simulador, false = hardware real
  POLL_INTERVAL: 100,         // ms (encoder)
  LIVE_INTERVAL: 1000,        // ms (config de PID/posição: running + saída)
  STATUS_INTERVAL: 2000,      // ms (status do sistema)
  CHART_MAX_POINTS: 300,      // ~30s a 100ms
  SYNC_DEBOUNCE_MS: 600,      // envio automático dos ganhos após digitar
};

// Nomes amigáveis dos modos do firmware (campo "mode" da /api/status)
const MODE_NAMES = {
  'open-loop': 'Manual',
  pid: 'PID Velocidade',
  position: 'Posição',
  autotune: 'Auto-Tune',
};

// Título do gráfico por aba
const CHART_TITLES = {
  speed: 'Resposta de Velocidade (RPM)',
  position: 'Resposta de Posição (graus)',
  'open-loop': 'Velocidade Medida (RPM)',
};

// ============================================================
// SIMULADOR MOCK
// ============================================================

class Simulator {
  constructor() {
    this.time = 0;
    this.dt = CONFIG.POLL_INTERVAL / 1000;   // tick da UI (0.1 s)
    // O firmware real controla a 50 Hz (PID_SAMPLE_MS). Simular o laço
    // na taxa do gráfico (10 Hz) exagera o atraso de fase e faz os
    // ganhos do auto-tune oscilarem em ciclo-limite sustentado — na
    // bancada eles convergem. Então: física + PID em SUB_PASSOS de
    // 20 ms por tick de 100 ms da interface.
    this.SUB_PASSOS = 5;
    this.dtCtrl = this.dt / this.SUB_PASSOS; // 0.02 s (50 Hz)

    // Estado do motor
    this.rpm = 0;
    this.angle = 0;
    this.pulses = 0;
    this.pulseAcc = 0;
    this.pwmTarget = 0;     // PWM alvo (0-100)
    this.direction = 0;     // -1, 0, +1

    // Modelo do motor (first-order)
    this.tau = 0.4;         // constante de tempo (s)
    this.noiseLevel = 1.5;  // ruído em RPM
    this.maxRPM = 1950;     // RPM a 100% PWM (medido na bancada)
    // NOTA: sem fila de atraso — o firmware real não tem pipeline de
    // comandos, e uma fila no mock fazia o freio deixar alvos obsoletos
    // que "teleportavam" o motor no próximo acionamento (caça divergente).
    // A inércia já está no tau da 1ª ordem.

    // PID de velocidade
    this.pidEnabled = false;
    this.setpoint = 1000;
    this.kp = 0;
    this.ki = 0;
    this.kd = 0;
    this.integral = 0;
    this.prevRpm = 0;
    this.pidOutput = 0;

    // PID de posição (mock simples, com zona morta como na bancada)
    this.posEnabled = false;
    this.posTarget = 90;
    this.posKp = 1;
    this.posKi = 0.2;
    this.posKd = 0.05;
    this.posIntegral = 0;
    this.posPrevAngle = 0;
    this.posOutput = 0;
    this._softPhase = 0;  // fase do chute suave (quota/pausa)

    // Auto-tune
    this.autotuneRunning = false;
    this.autotunePlant = 'speed';
    this.autotuneRelayAmp = 18;
    this.autotuneBias = 82;
    this.autotuneCycles = 3;
    this.autotuneOscillations = [];
    this.autotuneLastCross = 0;
    this.autotuneCrossCount = 0;
    this.autotuneStartTime = 0;
    this.autotuneProgress = 0;
    this.autotuneResults = null;
    this.autotunePrevSign = 1;
    this.autotuneRelayOutput = 0;

    // Histórico para resposta ao degrau
    this.responseData = [];
    this.responseUnit = 'rpm';
    this.responseStartTime = 0;
  }

  noise() {
    return (Math.random() - 0.5) * 2 * this.noiseLevel;
  }

  update() {
    // 5 sub-passos de 20 ms (50 Hz) por tick de 100 ms da UI — a mesma
    // taxa de controle do firmware real. O gráfico continua a 10 Hz.
    for (let i = 0; i < this.SUB_PASSOS; i++) {
      const dt = this.dtCtrl;
      this.time += dt;

      if (this.autotuneRunning) {
        this._updateAutotune(dt);
      } else if (this.pidEnabled) {
        this._updatePID(dt);
      } else if (this.posEnabled) {
        this._updatePosition(dt);
      } else {
        this._updateOpenLoop(dt);
      }

      // Integrar ângulo e pulsos (600 PPR) por sub-passo
      this.angle += (this.rpm / 60) * 360 * dt;
      this.pulseAcc += (this.rpm / 60) * 600 * dt;
      this.pulses = Math.round(this.pulseAcc);
    }

    // Registrar dados para resposta ao degrau (1 amostra por tick da UI)
    if (this.autotuneRunning && this.autotunePlant === 'position') {
      this._record(this.posTarget, this.angle);
    } else if (this.pidEnabled || this.autotuneRunning) {
      this._record(this.setpoint, Math.abs(this.rpm));
    } else if (this.posEnabled) {
      this._record(this.posTarget, this.angle);
    }
  }

  _record(sp, value) {
    this.responseData.push([this.time - this.responseStartTime, sp, value]);
    if (this.responseData.length > 1000) this.responseData.shift();
  }

  _applyMotor(targetRPM, dt, noiseScale) {
    // Aplicação imediata do alvo (sem fila): a 1ª ordem com tau faz o
    // papel da inércia/atraso da bancada.
    this.rpm += (targetRPM - this.rpm) * (dt / this.tau) + this.noise() * dt * noiseScale;
    this.rpm = Math.max(-this.maxRPM, Math.min(this.maxRPM, this.rpm));
  }

  /**
   * Planta do motor do mock — FIEL À BANCADA REAL (L298N + motor DC):
   *  - Zona morta ~60% de duty (medida): abaixo disso o PWM não vence o
   *    atrito estático — sem torque, o eixo apenas roda em inércia
   *    (STOP do L298N = roda-livre, pins LOW).
   *  - 100% de duty ≈ 1950 RPM (medido); acionamento mínimo (logo acima
   *    da zona morta) move ~230 RPM.
   *  - Resposta de primeira ordem (tau) tanto acelerando quanto rodando
   *    livre até o repouso.
   * `duty` em % (negativo = anti-horário).
   */
  _motorPlant(duty, dt) {
    const deadZone = 60;
    const mag = Math.abs(duty);
    let target = 0;
    if (mag > deadZone) {
      target = ((mag - deadZone) / (100 - deadZone)) * this.maxRPM;
    }
    this._applyMotor(target * Math.sign(duty), dt, 5);
  }

  /**
   * Freio dinâmico do L298N (fases curtocircuitadas): o eixo para quase
   * instantaneamente — mata a inércia do coast no bang-bang da posição.
   * Na bancada real o freio é praticamente imediato (erro assenta em ±2°).
   */
  _applyBrake(dt) {
    const tauBrake = 0.008; // s — freio do L298N quase instantâneo
    // Forma exponencial: estável para qualquer dt (o fator linear
    // 1 - dt/tau fica negativo quando dt > tau e explode o sinal)
    this.rpm *= Math.exp(-dt / tauBrake);
  }

  _updateOpenLoop(dt) {
    // Malha aberta: duty = PWM manual, com a zona morta da bancada
    const duty = this.pwmTarget * this.direction;
    this._motorPlant(duty, dt);
  }

  _updatePID(dt) {
    const rpmAbs = Math.abs(this.rpm);
    const error = this.setpoint - rpmAbs;

    // Anti-windup simples
    this.integral += error * dt;
    const integralMax = 100 / (this.ki > 0 ? this.ki : 1);
    this.integral = Math.max(-integralMax, Math.min(integralMax, this.integral));

    // Derivative on measurement
    const derivative = -(rpmAbs - this.prevRpm) / dt;

    let output = this.kp * error + this.ki * this.integral + this.kd * derivative;
    this.pidOutput = Math.max(0, Math.min(100, output));

    // Motor unidirecional (horário), como no firmware
    this._motorPlant(this.pidOutput, dt);
    this.prevRpm = Math.abs(this.rpm);
  }

  _updatePosition(dt) {
    const error = this.posTarget - this.angle;

    // Zona morta didática em torno do alvo (como no firmware real)
    if (Math.abs(error) <= 2.0) {
      this.posOutput = 0;
      this.posIntegral += error * dt;
      this._applyBrake(dt);
      return;
    }

    this.posIntegral += error * dt;
    // Teto do termo integral (como no firmware): |Ki·I| ≤ 55% — abaixo da
    // zona morta do motor (60%), para o I sozinho não manter o motor
    // acionado com erro ~0 (rastejo além do alvo).
    const integralMax = 55 / (this.posKi > 0 ? this.posKi : 1);
    this.posIntegral = Math.max(-integralMax, Math.min(integralMax, this.posIntegral));
    // Teto do termo derivativo (como no firmware): em alta velocidade o
    // de/dt explode e o D sozinho inverteria a saída a cada tick. O clamp
    // é na CONTRIBUIÇÃO (em % de saída), não no de/dt cru.
    const derivative = -(this.angle - this.posPrevAngle) / dt;
    let dTerm = this.posKd * derivative;
    dTerm = Math.max(-30, Math.min(30, dTerm));

    const output = this.posKp * error + this.posKi * this.posIntegral + dTerm;
    this.posOutput = Math.max(-100, Math.min(100, output));

    // Atuação em camadas (idêntica ao controlTick do firmware):
    if (Math.abs(error) <= 2.0 || Math.abs(this.posOutput) < 1) {
      // Deadband ou PID pedindo repouso: freio (trava o eixo)
      this._applyBrake(dt);
      this._softPhase = 0;
    } else if (Math.abs(error) < 60 && Math.abs(this.rpm) > 100) {
      // Frenagem antecipada: em movimento e chegando perto — freia antes
      // de entrar na zona de passos (mata a velocidade de entrada)
      this._applyBrake(dt);
      this._softPhase = 0;
    } else if (Math.abs(error) < 30) {
      // Perto do alvo: passos discretos — [chute 1 tick a 70%] →
      // [freio 8 ticks (descanso do driver)] → repete, direção pelo
      // SINAL DO ERRO
      if (this._softPhase < 1) {
        this._motorPlant(70 * Math.sign(error), dt);
        this._softPhase++;
      } else if (this._softPhase < 9) {
        this._applyBrake(dt);
        this._softPhase++;
      } else {
        this._softPhase = 0;
      }
    } else {
      // Longe do alvo: duty fixo 70% (região confiável acima do atrito)
      this._softPhase = 0;
      this._motorPlant(70 * Math.sign(this.posOutput), dt);
    }
    this.posPrevAngle = this.angle;
  }

  _updateAutotune(dt) {
    if (this.autotunePlant === 'position') {
      this._updateAutotunePosition(dt);
    } else {
      this._updateAutotuneSpeed(dt);
    }
  }

  _updateAutotuneSpeed(dt) {
    const elapsed = this.time - this.autotuneStartTime;
    const totalDuration = this.autotuneCycles * 2 * 1.2; // estimativa didática

    this.autotuneProgress = Math.min(100, (elapsed / totalDuration) * 100);

    const rpmAbs = Math.abs(this.rpm);
    const sign = rpmAbs < this.setpoint ? 1 : -1;

    if (sign !== this.autotunePrevSign) {
      this.autotuneCrossCount++;
      // A partir do 2º cruzamento, reinicia o envelope (o transiente de
      // partida não faz parte do ciclo limite) — como no firmware.
      if (this.autotuneCrossCount === 2) {
        this.autotuneMeasMin = rpmAbs;
        this.autotuneMeasMax = rpmAbs;
      }
      if (this.autotuneCrossCount >= 2) {
        const halfPeriod = this.time - this.autotuneLastCross;
        this.autotuneOscillations.push(halfPeriod * 2);
      }
      this.autotuneLastCross = this.time;
      this.autotunePrevSign = sign;
    }

    // Envelope real da oscilação (como o firmware mede)
    if (this.autotuneCrossCount >= 2) {
      if (rpmAbs < this.autotuneMeasMin) this.autotuneMeasMin = rpmAbs;
      if (rpmAbs > this.autotuneMeasMax) this.autotuneMeasMax = rpmAbs;
    }

    // Relay com bias: bias ± amplitude (como no firmware real)
    this.autotuneRelayOutput = sign > 0
      ? Math.min(100, this.autotuneBias + this.autotuneRelayAmp)
      : Math.max(0, this.autotuneBias - this.autotuneRelayAmp);

    // Planta real da bancada (zona morta ~60%)
    this._motorPlant(this.autotuneRelayOutput, dt);

    if (this.autotuneCrossCount >= this.autotuneCycles * 2 + 2 || elapsed > 30) {
      this._finishAutotune();
    }
  }

  _updateAutotunePosition(dt) {
    const elapsed = this.time - this.autotuneStartTime;

    this.autotuneProgress = Math.min(100, (elapsed / (this.autotuneCycles * 2 * 2.5)) * 100);

    // Relé simétrico em torno do alvo em graus: abaixo do alvo → +d (horário),
    // em/abaixo → -d (anti-horário). A oscilação do ângulo é o ciclo limite.
    const error = this.posTarget - this.angle;
    const sign = error > 0 ? 1 : -1;

    if (sign !== this.autotunePrevSign) {
      this.autotuneCrossCount++;
      if (this.autotuneCrossCount === 2) {
        this.autotuneMeasMin = this.angle;
        this.autotuneMeasMax = this.angle;
      }
      if (this.autotuneCrossCount >= 2) {
        const halfPeriod = this.time - this.autotuneLastCross;
        this.autotuneOscillations.push(halfPeriod * 2);
      }
      this.autotuneLastCross = this.time;
      this.autotunePrevSign = sign;
    }

    if (this.autotuneCrossCount >= 2) {
      if (this.angle < this.autotuneMeasMin) this.autotuneMeasMin = this.angle;
      if (this.angle > this.autotuneMeasMax) this.autotuneMeasMax = this.angle;
    }

    this.autotuneRelayOutput = sign > 0 ? this.autotuneRelayAmp : -this.autotuneRelayAmp;

    // Planta real da bancada: no motor a ±90% o eixo balança múltiplas
    // voltas por semiciclo — o ciclo limite é grande mesmo (o gráfico
    // mostra isso de verdade).
    this._motorPlant(this.autotuneRelayOutput, dt);

    if (this.autotuneCrossCount >= this.autotuneCycles * 2 + 2 || elapsed > 45) {
      this._finishAutotune();
    }
  }

  _finishAutotune() {
    this.autotuneRunning = false;
    this.autotuneRelayOutput = 0;

    let tu = this.autotuneOscillations.length > 0
      ? this.autotuneOscillations.reduce((a, b) => a + b, 0) / this.autotuneOscillations.length
      : 1.5;

    // Amplitude REAL do ciclo limite, medida do envelope (como o firmware).
    // Fallback para estimativa se o envelope não foi coletado.
    let a;
    if (this.autotunePlant === 'position') {
      a = (this.autotuneMeasMax - this.autotuneMeasMin) / 2;
      if (!(a > 1)) a = Math.max(5, Math.abs(this.posTarget) * 0.25);
      const ku = (4 * this.autotuneRelayAmp) / (Math.PI * a);
      this.autotuneResults = {
        ku: parseFloat(ku.toFixed(2)),
        tu: parseFloat(tu.toFixed(3)),
        zn: this._calcZN(ku, tu),
        tl: this._calcTL(ku, tu),
        cc: this._calcCC(ku, tu),
      };
    } else {
      a = (this.autotuneMeasMax - this.autotuneMeasMin) / 2;
      if (!(a > 1)) a = this.maxRPM * 0.15;
      const ku = (4 * this.autotuneRelayAmp) / (Math.PI * a);
      this.autotuneResults = {
        ku: parseFloat(ku.toFixed(2)),
        tu: parseFloat(tu.toFixed(3)),
        zn: this._calcZN(ku, tu),
        tl: this._calcTL(ku, tu),
        cc: this._calcCC(ku, tu),
      };
    }
    this.autotuneProgress = 100;
  }

  _calcZN(ku, tu) {
    return {
      kp: parseFloat((0.6 * ku).toFixed(2)),
      ki: parseFloat(((0.6 * ku * 2) / tu).toFixed(2)),
      kd: parseFloat(((0.6 * ku * tu) / 8).toFixed(3)),
    };
  }

  _calcTL(ku, tu) {
    return {
      kp: parseFloat((ku / 2.2).toFixed(2)),
      ki: parseFloat(((ku / 2.2) / (2.2 * tu)).toFixed(2)),
      kd: parseFloat(((ku / 2.2) * tu / 6.3).toFixed(3)),
    };
  }

  _calcCC(ku, tu) {
    return {
      kp: parseFloat((ku / 1.35).toFixed(2)),
      ki: parseFloat(((ku / 1.35) / (tu * 0.5)).toFixed(2)),
      kd: parseFloat(((ku / 1.35) * tu * 0.25).toFixed(3)),
    };
  }

  startAutotune(plant, relayAmp, bias, cycles, setpoint) {
    this.autotunePlant = plant;
    this.autotuneRunning = true;
    this.autotuneRelayAmp = relayAmp;
    this.autotuneBias = bias;
    this.autotuneCycles = cycles;
    this.pidEnabled = false;
    this.posEnabled = false;
    this.autotuneStartTime = this.time;
    this.autotuneOscillations = [];
    this.autotuneMeasMin = Infinity;
    this.autotuneMeasMax = -Infinity;
    this.autotuneCrossCount = 0;
    this.autotuneLastCross = this.time;
    this.autotuneProgress = 0;
    this.autotuneResults = null;
    this.autotunePrevSign = 1;
    this.responseData = [];
    this.responseStartTime = this.time;
    if (plant === 'position') {
      this.posTarget = setpoint;
      this.responseUnit = 'deg';
      this._record(this.posTarget, this.angle);
    } else {
      this.setpoint = setpoint;
      this.responseUnit = 'rpm';
      this.rpm = this.setpoint * 0.4;
      this._record(this.setpoint, Math.abs(this.rpm));
    }
  }

  startPID() {
    this.pidEnabled = true;
    this.autotuneRunning = false;
    this.posEnabled = false;
    this.integral = 0;
    this.prevRpm = Math.abs(this.rpm);
    this.responseData = [];
    this.responseUnit = 'rpm';
    this.responseStartTime = this.time;
  }

  stopPID() {
    this.pidEnabled = false;
    this.pidOutput = 0;
  }

  startPos(target) {
    this.posEnabled = true;
    this.posTarget = target;
    this.pidEnabled = false;
    this.autotuneRunning = false;
    this.posIntegral = 0;
    this.posPrevAngle = this.angle;
    this._softPhase = 0;
    this.responseData = [];
    this.responseUnit = 'deg';
    this.responseStartTime = this.time;
  }

  stopPos() {
    this.posEnabled = false;
    this.posOutput = 0;
  }

  zeroPos() {
    this.angle = 0;
    this.pulses = 0;
  }

  getEncoder() {
    return {
      pulses: this.pulses,
      rpm: parseFloat(this.rpm.toFixed(1)),
      angle: parseFloat(this.angle.toFixed(1)),
    };
  }

  getPidConfig() {
    const rpmAbs = Math.abs(this.rpm);
    const running = this.pidEnabled;
    return {
      kp: this.kp,
      ki: this.ki,
      kd: this.kd,
      setpoint: this.setpoint,
      running: running,
      measurement: running ? parseFloat(rpmAbs.toFixed(1)) : 0,
      error: running ? parseFloat((this.setpoint - rpmAbs).toFixed(1)) : 0,
      output: running ? parseFloat(this.pidOutput.toFixed(1)) : 0,
      mode: running ? 'pid' : 'open-loop',
    };
  }

  getPosConfig() {
    const running = this.posEnabled;
    return {
      kp: this.posKp,
      ki: this.posKi,
      kd: this.posKd,
      target: this.posTarget,
      angle: parseFloat(this.angle.toFixed(1)),
      running: running,
      error: running ? parseFloat((this.posTarget - this.angle).toFixed(1)) : 0,
      output: running ? parseFloat(this.posOutput.toFixed(1)) : 0,
      mode: running ? 'position' : 'open-loop',
    };
  }

  getStatus() {
    return {
      version: '0.1.0-mock',
      wifi_rssi: -42,
      stations: 1,
      ssid: 'GIRINO_AP',
      ip: '192.168.4.1',
      uptime_ms: Math.round(this.time * 1000),
      free_heap: 32768,
      angle: parseFloat(this.angle.toFixed(1)),
      mode: this.autotuneRunning ? 'autotune' : (this.posEnabled ? 'position' : (this.pidEnabled ? 'pid' : 'open-loop')),
    };
  }

  getAutotuneStatus() {
    return {
      status: this.autotuneRunning ? 'running' : (this.autotuneResults ? 'done' : 'idle'),
      plant: this.autotunePlant,
      progress: this.autotuneProgress,
      output: this.autotuneRunning ? this.autotuneRelayOutput : 0,
      results: this.autotuneResults,
    };
  }

  getTuning() {
    if (!this.autotuneResults) return null;
    return {
      plant: this.autotunePlant,
      ku: this.autotuneResults.ku,
      tu: this.autotuneResults.tu,
      zn: this.autotuneResults.zn,
      tl: this.autotuneResults.tl,
      cc: this.autotuneResults.cc,
    };
  }

  getPidResponse() {
    return {
      data: this.responseData.slice(-CONFIG.CHART_MAX_POINTS),
      setpoint: this.responseUnit === 'deg' ? this.posTarget : this.setpoint,
      unit: this.responseUnit,
    };
  }

  motorCommand(direction, speed) {
    this.direction = direction === 'forward' ? 1 : (direction === 'reverse' ? -1 : 0);
    this.pwmTarget = speed;
    if (direction === 'stop') {
      this.pidEnabled = false;
      this.posEnabled = false;
      this.autotuneRunning = false;
    }
  }
}

// ============================================================
// API CLIENT
// ============================================================

class APIClient {
  constructor(simulator) {
    this.simulator = simulator;
    this.connected = true;
    this.failCount = 0;
    // Estado local (espelhado do firmware) para a UI
    this.setpoint = 1000;
    this.pidActive = false;
    this.autotuneActive = false;
    this.posActive = false;
    this.posTarget = 0;
  }

  async _fetch(url, options = {}) {
    if (CONFIG.USE_MOCK) return null;
    try {
      const res = await fetch(url, options);
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      this.failCount = 0;
      this.connected = true;
      return await res.json();
    } catch (e) {
      this.failCount++;
      if (this.failCount >= 3) this.connected = false;
      return null;
    }
  }

  async getEncoder() {
    if (CONFIG.USE_MOCK) return this.simulator.getEncoder();
    return await this._fetch('/api/encoder');
  }

  async getPidConfig() {
    if (CONFIG.USE_MOCK) return this.simulator.getPidConfig();
    return await this._fetch('/api/pid/config');
  }

  async getPosConfig() {
    if (CONFIG.USE_MOCK) return this.simulator.getPosConfig();
    return await this._fetch('/api/position/config');
  }

  async setPidConfig(kp, ki, kd, setpoint) {
    this.setpoint = setpoint;
    if (CONFIG.USE_MOCK) {
      this.simulator.kp = kp;
      this.simulator.ki = ki;
      this.simulator.kd = kd;
      this.simulator.setpoint = setpoint;
      return { ok: true };
    }
    return await this._fetch('/api/pid/config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: `kp=${kp}&ki=${ki}&kd=${kd}&setpoint=${setpoint}`,
    });
  }

  async startPid() {
    if (CONFIG.USE_MOCK) {
      this.simulator.startPID();
      this.pidActive = true;
      this.autotuneActive = false;
      this.posActive = false;
      return { ok: true };
    }
    const res = await this._fetch('/api/pid/start', { method: 'POST' });
    if (res) {
      this.pidActive = true;
      this.autotuneActive = false;
      this.posActive = false;
    }
    return res;
  }

  async stopPid() {
    if (CONFIG.USE_MOCK) {
      this.simulator.stopPID();
      this.pidActive = false;
      return { ok: true };
    }
    const res = await this._fetch('/api/pid/stop', { method: 'POST' });
    if (res) this.pidActive = false;
    return res;
  }

  // --- Controle de posição (ângulo) ---

  async setPosConfig(kp, ki, kd, target) {
    this.posTarget = target;
    if (CONFIG.USE_MOCK) {
      this.simulator.posKp = kp;
      this.simulator.posKi = ki;
      this.simulator.posKd = kd;
      this.simulator.posTarget = target;
      return { ok: true };
    }
    return await this._fetch('/api/position/config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: `kp=${kp}&ki=${ki}&kd=${kd}&target=${target}`,
    });
  }

  async startPos(target) {
    if (target !== undefined) this.posTarget = target;
    if (CONFIG.USE_MOCK) {
      this.simulator.startPos(this.posTarget);
      this.posActive = true;
      this.pidActive = false;
      return { ok: true };
    }
    const res = await this._fetch('/api/position/start', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: `target=${this.posTarget}`,
    });
    if (res) {
      this.posActive = true;
      this.pidActive = false;
      this.autotuneActive = false;
    }
    return res;
  }

  async stopPos() {
    if (CONFIG.USE_MOCK) {
      this.simulator.stopPos();
      this.posActive = false;
      return { ok: true };
    }
    const res = await this._fetch('/api/position/stop', { method: 'POST' });
    if (res) this.posActive = false;
    return res;
  }

  async zeroPos() {
    if (CONFIG.USE_MOCK) {
      this.simulator.zeroPos();
      return { ok: true };
    }
    return await this._fetch('/api/position/zero', { method: 'POST' });
  }

  async startAutotune(relayAmplitude, cycles, setpoint, bias) {
    if (setpoint !== undefined) this.setpoint = setpoint;
    if (CONFIG.USE_MOCK) {
      this.simulator.startAutotune('speed', relayAmplitude, bias, cycles, setpoint);
      this.autotuneActive = true;
      this.pidActive = false;
      this.posActive = false;
      return { ok: true, plant: 'speed' };
    }
    const res = await this._fetch('/api/pid/autotune', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: `plant=speed&relay_amplitude=${relayAmplitude}&cycles=${cycles}&setpoint=${this.setpoint}&bias=${bias}`,
    });
    if (res) {
      this.autotuneActive = true;
      this.pidActive = false;
      this.posActive = false;
    }
    return res;
  }

  async startAutotunePosition(relayAmplitude, cycles, target) {
    if (target !== undefined) this.posTarget = target;
    if (CONFIG.USE_MOCK) {
      this.simulator.startAutotune('position', relayAmplitude, 0, cycles, target);
      this.autotuneActive = true;
      this.pidActive = false;
      this.posActive = false;
      return { ok: true, plant: 'position' };
    }
    const res = await this._fetch('/api/pid/autotune', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: `plant=position&relay_amplitude=${relayAmplitude}&cycles=${cycles}&target=${this.posTarget}`,
    });
    if (res) {
      this.autotuneActive = true;
      this.pidActive = false;
      this.posActive = false;
    }
    return res;
  }

  async getAutotuneStatus() {
    if (CONFIG.USE_MOCK) return this.simulator.getAutotuneStatus();
    return await this._fetch('/api/pid/autotune');
  }

  async getTuning() {
    if (CONFIG.USE_MOCK) return this.simulator.getTuning();
    return await this._fetch('/api/pid/tuning');
  }

  async applyTuning(method) {
    if (CONFIG.USE_MOCK) {
      const tuning = this.simulator.getTuning();
      if (tuning && tuning[method]) {
        this.simulator.kp = tuning[method].kp;
        this.simulator.ki = tuning[method].ki;
        this.simulator.kd = tuning[method].kd;
      }
      return { ok: true };
    }
    return await this._fetch('/api/pid/tuning/apply', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: `method=${method}`,
    });
  }

  async getPidResponse() {
    if (CONFIG.USE_MOCK) return this.simulator.getPidResponse();
    return await this._fetch('/api/pid/response');
  }

  async motorCommand(direction, speed) {
    if (CONFIG.USE_MOCK) {
      this.simulator.motorCommand(direction, speed);
      if (direction === 'stop') {
        this.pidActive = false;
        this.autotuneActive = false;
        this.posActive = false;
      }
      return { ok: true };
    }
    // Comando manual desliga PID/autotune/posição no firmware
    if (direction === 'stop') {
      this.pidActive = false;
      this.autotuneActive = false;
      this.posActive = false;
    }
    return await this._fetch('/api/motor', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: `direction=${direction}&speed=${speed}`,
    });
  }

  async getStatus() {
    if (CONFIG.USE_MOCK) return this.simulator.getStatus();
    return await this._fetch('/api/status');
  }

  async getEncoderForChart() {
    if (CONFIG.USE_MOCK) {
      this.simulator.update();
      return this.simulator.getEncoder();
    }
    return await this._fetch('/api/encoder');
  }
}

// ============================================================
// CHART MANAGER
// ============================================================

class ChartManager {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    this.chart = null;
    this.dataPoints = { time: [], setpoint: [], value: [] };
    this._init();
  }

  _init() {
    const isDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
    const gridColor = isDark ? 'rgba(255,255,255,0.08)' : 'rgba(0,0,0,0.06)';
    const textColor = isDark ? '#aaaaaa' : '#616161';

    this.chart = new Chart(this.canvas, {
      type: 'line',
      data: {
        labels: [],
        datasets: [
          {
            label: 'Medido',
            data: [],
            borderColor: '#4caf50',
            backgroundColor: 'rgba(76, 175, 80, 0.1)',
            borderWidth: 2,
            pointRadius: 0,
            tension: 0.3,
            fill: true,
          },
          {
            label: 'Setpoint',
            data: [],
            borderColor: isDark ? '#42a5f5' : '#1565c0',
            borderWidth: 2,
            borderDash: [8, 4],
            pointRadius: 0,
            tension: 0,
            fill: false,
            spanGaps: false,
          },
        ],
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: false,
        interaction: {
          mode: 'index',
          intersect: false,
        },
        plugins: {
          legend: {
            display: true,
            position: 'top',
            labels: {
              boxWidth: 16,
              padding: 8,
              font: { size: 11 },
              color: textColor,
              usePointStyle: true,
              pointStyle: 'line',
            },
          },
          tooltip: {
            enabled: true,
            backgroundColor: isDark ? '#333' : '#fff',
            titleColor: isDark ? '#eee' : '#333',
            bodyColor: isDark ? '#ccc' : '#555',
            borderColor: isDark ? '#555' : '#ddd',
            borderWidth: 1,
            padding: 8,
            titleFont: { size: 12 },
            bodyFont: { size: 11 },
            callbacks: {
              title: (items) => `t = ${items[0].label}s`,
            },
          },
        },
        scales: {
          x: {
            display: true,
            title: {
              display: true,
              text: 'Tempo (s)',
              font: { size: 11 },
              color: textColor,
            },
            ticks: {
              color: textColor,
              font: { size: 10 },
              maxTicksLimit: 8,
            },
            grid: { color: gridColor },
          },
          y: {
            display: true,
            title: {
              display: true,
              text: 'RPM',
              font: { size: 11 },
              color: textColor,
            },
            ticks: {
              color: textColor,
              font: { size: 10 },
            },
            grid: { color: gridColor },
            suggestedMin: 0,
          },
        },
      },
    });
  }

  addPoint(time, value, setpoint) {
    const maxPoints = CONFIG.CHART_MAX_POINTS;

    this.dataPoints.time.push(time.toFixed(1));
    this.dataPoints.value.push(value);
    this.dataPoints.setpoint.push(setpoint);

    if (this.dataPoints.time.length > maxPoints) {
      this.dataPoints.time.shift();
      this.dataPoints.value.shift();
      this.dataPoints.setpoint.shift();
    }

    this.chart.data.labels = this.dataPoints.time;
    this.chart.data.datasets[0].data = this.dataPoints.value;
    this.chart.data.datasets[1].data = this.dataPoints.setpoint;
    this.chart.update('none');
  }

  clear() {
    this.dataPoints = { time: [], setpoint: [], value: [] };
    this.chart.data.labels = [];
    this.chart.data.datasets[0].data = [];
    this.chart.data.datasets[1].data = [];
    this.chart.update();
  }

  /**
   * Ajusta rótulos e eixo para a grandeza medida:
   * 'rpm' (velocidade) ou 'deg' (posição/ângulo).
   */
  setUnits(unit) {
    const isDeg = unit === 'deg';
    this.chart.data.datasets[0].label = isDeg ? 'Ângulo Real (°)' : 'Velocidade Real (RPM)';
    this.chart.data.datasets[1].label = isDeg ? 'Alvo (°)' : 'Setpoint (RPM)';
    this.chart.options.scales.y.title.text = isDeg ? 'Ângulo (°)' : 'RPM';
    this.chart.update('none');
  }

  setSetpointVisible(visible) {
    const ds = this.chart.data.datasets[1];
    if (visible) {
      ds.hidden = false;
      this.chart.options.plugins.legend.labels.filter = undefined;
    } else {
      ds.hidden = true;
      this.dataPoints.setpoint = [];
      ds.data = [];
      this.chart.options.plugins.legend.labels.filter = (item) => item.datasetIndex !== 1;
    }
    this.chart.update('none');
  }

  exportCSV(unit) {
    const isDeg = unit === 'deg';
    let csv = `Tempo (s),Setpoint (${isDeg ? 'graus' : 'RPM'}),Medido (${isDeg ? 'graus' : 'RPM'})\n`;
    for (let i = 0; i < this.dataPoints.time.length; i++) {
      csv += `${this.dataPoints.time[i]},${this.dataPoints.setpoint[i]},${this.dataPoints.value[i]}\n`;
    }
    const blob = new Blob([csv], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `girino_${new Date().toISOString().slice(0, 19).replace(/:/g, '-')}.csv`;
    a.click();
    URL.revokeObjectURL(url);
  }
}

// ============================================================
// UI CONTROLLER
// ============================================================

class UIController {
  constructor(api, chart) {
    this.api = api;
    this.chart = chart;
    this.mode = 'speed';        // 'speed' | 'position' | 'open-loop'
    this.running = false;
    this.paused = false;
    this.chartTime = 0;
    // Valores default já casam com o firmware — leitura/SP visíveis
    // desde o início; qualquer edição sincroniza sozinha (debounce).
    this.setpointConfigured = true;
    this.posConfigured = true;
    this._pidSyncTimer = null;
    this._posSyncTimer = null;
    this._autotunePoll = null;
    this._lastPulses = null;
    this._lastPulseMs = 0;
    this._fwOutput = null;      // saída (%) vinda do firmware (PID/relé)
    this._fwRunning = false;

    this._cacheElements();
    this._bindEvents();
    this._setMode('speed');
    this._startPolling();
  }

  _cacheElements() {
    this.els = {
      // Header
      version: document.getElementById('version'),
      connDot: document.getElementById('conn-dot'),
      connLabel: document.getElementById('conn-label'),
      btnStopAll: document.getElementById('btn-stop-all'),

      // Abas
      tabs: document.querySelectorAll('.tab'),

      // Painéis de modo
      panelSpeed: document.getElementById('panel-speed'),
      panelPosition: document.getElementById('panel-position'),
      panelOpenLoop: document.getElementById('panel-open-loop'),

      // Badges de estado
      badgeSpeed: document.getElementById('badge-speed'),
      badgePosition: document.getElementById('badge-position'),
      badgeOpenLoop: document.getElementById('badge-open-loop'),

      // Leitura ao vivo — velocidade
      roSp: document.getElementById('ro-sp'),
      roRpm: document.getElementById('ro-rpm'),
      roErr: document.getElementById('ro-err'),
      roOut: document.getElementById('ro-out'),
      readoutSpeed: document.getElementById('readout-speed'),

      // Leitura ao vivo — posição
      roTarget: document.getElementById('ro-target'),
      roAngle: document.getElementById('ro-angle'),
      roPErr: document.getElementById('ro-perr'),
      roPOut: document.getElementById('ro-pout'),
      readoutPosition: document.getElementById('readout-position'),

      // Leitura ao vivo — manual
      roPwm: document.getElementById('ro-pwm'),
      roORpm: document.getElementById('ro-orpm'),

      // Gráfico
      chartTitle: document.getElementById('chart-title'),
      btnClearChart: document.getElementById('btn-clear-chart'),
      btnExportCsv: document.getElementById('btn-export-csv'),
      btnExportExp: document.getElementById('btn-export-exp'),

      // PID velocidade
      inputSetpoint: document.getElementById('input-setpoint'),
      inputKp: document.getElementById('input-kp'),
      inputKi: document.getElementById('input-ki'),
      inputKd: document.getElementById('input-kd'),
      btnRunSpeed: document.getElementById('btn-run-speed'),

      // Posição
      inputAngle: document.getElementById('input-angle'),
      inputPkp: document.getElementById('input-pkp'),
      inputPki: document.getElementById('input-pki'),
      inputPkd: document.getElementById('input-pkd'),
      btnRunPos: document.getElementById('btn-run-pos'),
      btnZero: document.getElementById('btn-zero'),

      // Auto-tune (velocidade)
      inputRelay: document.getElementById('input-relay'),
      inputBias: document.getElementById('input-bias'),
      inputCycles: document.getElementById('input-cycles'),
      btnAutotune: document.getElementById('btn-autotune'),
      progressContainer: document.getElementById('autotune-progress-container'),
      progressBar: document.getElementById('autotune-progress'),
      progressLabel: document.getElementById('autotune-progress-label'),
      autotuneResults: document.getElementById('autotune-results'),
      valKu: document.getElementById('val-ku'),
      valTu: document.getElementById('val-tu'),
      tuningSuggestions: document.getElementById('tuning-suggestions'),

      // Auto-tune (posição)
      inputPrelay: document.getElementById('input-prelay'),
      inputPcycles: document.getElementById('input-pcycles'),
      btnAutotunePos: document.getElementById('btn-autotune-pos'),
      progressPosContainer: document.getElementById('autotune-pos-progress-container'),
      progressBarPos: document.getElementById('autotune-pos-progress'),
      progressLabelPos: document.getElementById('autotune-pos-progress-label'),
      autotunePosResults: document.getElementById('autotune-pos-results'),
      valKuPos: document.getElementById('val-ku-pos'),
      valTuPos: document.getElementById('val-tu-pos'),
      tuningSuggestionsPos: document.getElementById('tuning-suggestions-pos'),

      // Manual
      inputSpeed: document.getElementById('input-speed'),
      speedDisplay: document.getElementById('speed-display'),
      btnForward: document.getElementById('btn-forward'),
      btnReverse: document.getElementById('btn-reverse'),

      // Status
      statMode: document.getElementById('stat-mode'),
      statClients: document.getElementById('stat-clients'),
      statEncRate: document.getElementById('stat-enc-rate'),
      statSsid: document.getElementById('stat-ssid'),
      statIp: document.getElementById('stat-ip'),
      statHeap: document.getElementById('stat-heap'),
      statUptime: document.getElementById('stat-uptime'),
      statPulses: document.getElementById('stat-pulses'),
    };
  }

  _bindEvents() {
    // Abas
    this.els.tabs.forEach((tab) => {
      tab.addEventListener('click', () => this._setMode(tab.dataset.mode));
    });

    // PARAR TUDO (header)
    this.els.btnStopAll.addEventListener('click', () => this._onStopAll());

    // PID velocidade — sincronização automática (debounce 600 ms)
    [this.els.inputSetpoint, this.els.inputKp, this.els.inputKi, this.els.inputKd]
      .forEach((el) => el.addEventListener('input', () => this._queuePidSync()));
    this.els.btnRunSpeed.addEventListener('click', () => this._onRunToggle());

    // Posição — sincronização automática (debounce 600 ms)
    [this.els.inputAngle, this.els.inputPkp, this.els.inputPki, this.els.inputPkd]
      .forEach((el) => el.addEventListener('input', () => this._queuePosSync()));
    this.els.btnZero.addEventListener('click', () => this._onZero());
    this.els.btnRunPos.addEventListener('click', () => this._onRunToggle());

    // Auto-tune
    this.els.btnAutotune.addEventListener('click', () => this._onAutotune());
    this.els.btnAutotunePos.addEventListener('click', () => this._onAutotunePos());

    // Manual
    this.els.inputSpeed.addEventListener('input', (e) => {
      this.els.speedDisplay.textContent = e.target.value;
      this.els.roPwm.textContent = e.target.value;
    });
    this.els.btnForward.addEventListener('click', () => this._onMotorCmd('forward'));
    this.els.btnReverse.addEventListener('click', () => this._onMotorCmd('reverse'));

    // Gráfico
    this.els.btnClearChart.addEventListener('click', () => {
      this.chart.clear();
      this.chartTime = 0;
    });
    this.els.btnExportCsv.addEventListener('click', () => {
      const unit = this.mode === 'position' ? 'deg' : 'rpm';
      this.chart.exportCSV(unit);
      this._toast('Dados exportados como CSV', 'success');
    });
    this.els.btnExportExp.addEventListener('click', () => this._onExportExperiment());
  }

  _setMode(mode) {
    // Trocar de aba cancela qualquer experimento em andamento
    if (this.mode !== mode &&
        (this.running || this.api.pidActive || this.api.autotuneActive || this.api.posActive)) {
      this._cancelExperiment();
    }

    this.mode = mode;

    // Painéis
    this.els.panelSpeed.classList.toggle('hidden', mode !== 'speed');
    this.els.panelPosition.classList.toggle('hidden', mode !== 'position');
    this.els.panelOpenLoop.classList.toggle('hidden', mode !== 'open-loop');

    // Abas
    this.els.tabs.forEach((t) => t.classList.toggle('active', t.dataset.mode === mode));

    // Gráfico por aba
    switch (mode) {
      case 'speed':
        this.chart.setUnits('rpm');
        this.chart.setSetpointVisible(true);
        break;
      case 'position':
        this.chart.setUnits('deg');
        this.chart.setSetpointVisible(true);
        break;
      case 'open-loop':
        this.chart.setUnits('rpm');
        this.chart.setSetpointVisible(false);
        break;
    }
    this.els.chartTitle.textContent = CHART_TITLES[mode] || 'Resposta em Tempo Real';

    this.chart.clear();
    this.chartTime = 0;
    this._updateButtons();
    this._updateReadoutIdle();
  }

  /**
   * Zera a leitura ao vivo ao trocar de aba (estado parado).
   */
  _updateReadoutIdle() {
    if (this.mode === 'speed') {
      this.els.roSp.textContent = '--';
      this.els.roRpm.textContent = '--';
      this.els.roErr.textContent = '--';
      this.els.roOut.textContent = '--';
      this._setErrClass(this.els.roErr, null);
      this.readoutSpeedIdle = true;
    } else if (this.mode === 'position') {
      this.els.roTarget.textContent = '--';
      this.els.roAngle.textContent = '--';
      this.els.roPErr.textContent = '--';
      this.els.roPOut.textContent = '--';
      this._setErrClass(this.els.roPErr, null);
      this.readoutPosIdle = true;
    }
  }

  _setErrClass(el, cls) {
    el.classList.remove('pos', 'neg', 'ok');
    if (cls) el.classList.add(cls);
  }

  _cancelExperiment() {
    if (this._autotunePoll) {
      clearInterval(this._autotunePoll);
      this._autotunePoll = null;
    }
    this.api.pidActive = false;
    this.api.autotuneActive = false;
    this.api.posActive = false;
    this.els.btnAutotune.disabled = false;
    this.els.btnAutotunePos.disabled = false;
    this.els.progressContainer.classList.add('hidden');
    this.els.progressPosContainer.classList.add('hidden');
    this.api.motorCommand('stop', 0); // firmware cancela todos os modos
    this.running = false;
    this.paused = false;
    this._fwOutput = null;
    this._fwRunning = false;
    this._updateButtons();
    this._updateBadges();
  }

  /**
   * Botão PARAR do header: mata motor + todos os controladores.
   */
  async _onStopAll() {
    await this._cancelExperiment();
    this._toast('Motor parado', 'info');
  }

  /**
   * Alterna Iniciar/Parar do botão da aba ativa.
   */
  _onRunToggle() {
    if (this.running) {
      this._onStop();
    } else {
      this._onStart();
    }
  }

  async _onStart() {
    if (this.mode === 'speed') {
      await this._syncPid();
      if ((parseFloat(this.els.inputKp.value) || 0) === 0 &&
          (parseFloat(this.els.inputKi.value) || 0) === 0 &&
          (parseFloat(this.els.inputKd.value) || 0) === 0) {
        this._toast('Aviso: ganhos zerados — o motor não vai se mover', 'warning');
      }
      const res = await this.api.startPid();
      if (!res && !CONFIG.USE_MOCK) {
        this._toast('Falha ao iniciar PID', 'error');
        return;
      }
      this.running = true;
      this.paused = false;
      this._toast('PID iniciado', 'success');
    } else if (this.mode === 'position') {
      await this._syncPos();
      const target = parseFloat(this.els.inputAngle.value) || 0;
      const res = await this.api.startPos(target);
      if (!res) {
        this._toast('Falha ao iniciar controle de posição', 'error');
        return;
      }
      this.running = true;
      this.paused = false;
      this._toast('Controle de posição iniciado', 'success');
    } else if (this.mode === 'open-loop') {
      const speed = parseInt(this.els.inputSpeed.value);
      this.api.motorCommand('forward', speed);
      this.running = true;
      this.paused = false;
      this._toast('Motor ligado (malha aberta)', 'success');
    }
    this._updateButtons();
    this._updateBadges();
    this.chart.clear();
    this.chartTime = 0;
  }

  async _onStop() {
    if (this.mode === 'speed') {
      await this.api.stopPid();
    } else if (this.mode === 'position') {
      await this.api.stopPos();
    }
    this._cancelExperiment();
    this._toast('Parado', 'info');
  }

  /**
   * Sincronização automática dos ganhos de velocidade (sem botão):
   * chamada 600 ms após a última tecla nos campos de Kp/Ki/Kd/setpoint.
   * "Iniciar" também força uma sincronia antes de ligar a malha.
   */
  _queuePidSync() {
    clearTimeout(this._pidSyncTimer);
    this._pidSyncTimer = setTimeout(() => this._syncPid(), CONFIG.SYNC_DEBOUNCE_MS);
  }

  _queuePosSync() {
    clearTimeout(this._posSyncTimer);
    this._posSyncTimer = setTimeout(() => this._syncPos(), CONFIG.SYNC_DEBOUNCE_MS);
  }

  async _syncPid() {
    clearTimeout(this._pidSyncTimer);
    const kp = parseFloat(this.els.inputKp.value) || 0;
    const ki = parseFloat(this.els.inputKi.value) || 0;
    const kd = parseFloat(this.els.inputKd.value) || 0;
    const setpoint = parseFloat(this.els.inputSetpoint.value) || 1000;
    await this.api.setPidConfig(kp, ki, kd, setpoint);
    this.setpointConfigured = true; // SP + erro aparecem no readout/gráfico
  }

  async _syncPos() {
    clearTimeout(this._posSyncTimer);
    const kp = parseFloat(this.els.inputPkp.value) || 0;
    const ki = parseFloat(this.els.inputPki.value) || 0;
    const kd = parseFloat(this.els.inputPkd.value) || 0;
    const target = parseFloat(this.els.inputAngle.value) || 0;
    await this.api.setPosConfig(kp, ki, kd, target);
    this.posConfigured = true; // alvo + erro aparecem no readout/gráfico
  }

  async _onZero() {
    const res = await this.api.zeroPos();
    if (!res) {
      this._toast('Falha ao zerar posição', 'error');
      return;
    }
    this.running = false;
    this.paused = false;
    this.posConfigured = true;
    this._updateButtons();
    this._updateBadges();
    this._toast('Posição atual definida como 0°', 'success');
  }

  async _onExportExperiment() {
    const data = await this.api.getPidResponse();
    if (!data || !data.data || data.data.length === 0) {
      this._toast('Nenhum experimento gravado no ESP ainda', 'warning');
      return;
    }
    const unit = data.unit === 'deg' ? 'Angulo (graus)' : 'RPM';
    let csv = `Tempo (s),Setpoint (${unit}),Medido (${unit})\n`;
    for (const [t, sp, value] of data.data) {
      csv += `${t},${sp},${value}\n`;
    }
    const blob = new Blob([csv], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `girino_experimento_${new Date().toISOString().slice(0, 19).replace(/:/g, '-')}.csv`;
    a.click();
    URL.revokeObjectURL(url);
    this._toast(`Experimento exportado (${data.data.length} amostras)`, 'success');
  }

  /**
   * Executa o auto-tune da planta indicada e faz o polling do progresso.
   * Os dois cards (velocidade e posição) compartilham a mesma lógica.
   */
  async _runAutotune(panel, startFn) {
    panel.btn.disabled = true;
    panel.container.classList.remove('hidden');
    panel.results.classList.add('hidden');
    panel.bar.style.width = '0%';
    panel.label.textContent = '0%';

    const res = await startFn();
    if (!res) {
      panel.btn.disabled = false;
      panel.container.classList.add('hidden');
      this._toast('Falha ao iniciar Auto-Tune', 'error');
      return;
    }

    this.running = true;
    this.paused = false;
    this._updateButtons();
    this._updateBadges();
    this._toast('Auto-Tune iniciado — o motor vai oscilar', 'info');
    this.chart.clear();
    this.chartTime = 0;

    if (this._autotunePoll) clearInterval(this._autotunePoll);
    this._autotunePoll = setInterval(async () => {
      const status = await this.api.getAutotuneStatus();
      if (!status) return;

      panel.bar.style.width = `${status.progress}%`;
      panel.label.textContent = `${Math.round(status.progress)}%`;

      // Saída do relé no readout ao vivo (± na posição)
      if (this.mode === 'position' && this.els.roPOut) {
        this.els.roPOut.textContent = (status.output ?? 0).toFixed(0);
      } else if (this.mode === 'speed') {
        this.els.roOut.textContent = (status.output ?? 0).toFixed(0);
      }

      if (status.status === 'done' && status.results) {
        clearInterval(this._autotunePoll);
        this._autotunePoll = null;
        this.running = false;
        panel.btn.disabled = false;
        panel.container.classList.add('hidden');
        this._updateButtons();
        this._updateBadges();
        this._showAutotuneResults(status.results, panel);
        this._toast('Auto-Tune concluído! Escolha um método abaixo.', 'success');
      } else if (status.status === 'failed') {
        clearInterval(this._autotunePoll);
        this._autotunePoll = null;
        this.running = false;
        panel.btn.disabled = false;
        panel.container.classList.add('hidden');
        this._updateButtons();
        this._updateBadges();
        this._toast('Auto-Tune falhou: oscilação insuficiente', 'error');
      }
    }, 200);
  }

  async _onAutotune() {
    const relayAmp = parseInt(this.els.inputRelay.value) || 18;
    const bias = parseInt(this.els.inputBias.value) || 82;
    const cycles = parseInt(this.els.inputCycles.value) || 3;
    const setpoint = parseFloat(this.els.inputSetpoint.value) || 1000;

    this.setpointConfigured = true; // linha do setpoint no gráfico
    await this._runAutotune({
      plant: 'speed',
      btn: this.els.btnAutotune,
      container: this.els.progressContainer,
      bar: this.els.progressBar,
      label: this.els.progressLabel,
      results: this.els.autotuneResults,
      valKu: this.els.valKu,
      valTu: this.els.valTu,
      suggestions: this.els.tuningSuggestions,
      inputs: [this.els.inputKp, this.els.inputKi, this.els.inputKd],
    }, () => this.api.startAutotune(relayAmp, cycles, setpoint, bias));
  }

  async _onAutotunePos() {
    const relayAmp = parseInt(this.els.inputPrelay.value) || 90;
    const cycles = parseInt(this.els.inputPcycles.value) || 3;
    const target = parseFloat(this.els.inputAngle.value) || 0;

    if (relayAmp < 65) {
      this._toast('Amplitude abaixo de ~60% (zona morta): o motor pode não oscilar', 'warning');
    }

    this.posConfigured = true; // linha do alvo no gráfico
    await this._runAutotune({
      plant: 'position',
      btn: this.els.btnAutotunePos,
      container: this.els.progressPosContainer,
      bar: this.els.progressBarPos,
      label: this.els.progressLabelPos,
      results: this.els.autotunePosResults,
      valKu: this.els.valKuPos,
      valTu: this.els.valTuPos,
      suggestions: this.els.tuningSuggestionsPos,
      inputs: [this.els.inputPkp, this.els.inputPki, this.els.inputPkd],
    }, () => this.api.startAutotunePosition(relayAmp, cycles, target));
  }

  _showAutotuneResults(results, panel) {
    panel.results.classList.remove('hidden');
    panel.valKu.textContent = panel.plant === 'position' ? `${results.ku} %/°` : results.ku;
    panel.valTu.textContent = `${results.tu}s`;

    const suggestions = panel.suggestions;
    suggestions.innerHTML = '';

    const methods = [
      { key: 'zn', name: 'Ziegler-Nichols', desc: 'resposta agressiva', data: results.zn },
      { key: 'tl', name: 'Tyreus-Luyben', desc: 'menos overshoot', data: results.tl },
      { key: 'cc', name: 'Cohen-Coon', desc: 'bom p/ atraso grande', data: results.cc },
    ];

    methods.forEach((m) => {
      const btn = document.createElement('button');
      btn.className = 'tuning-btn';
      btn.innerHTML = `
        <span class="tuning-name">${m.name}<br><small>${m.desc}</small></span>
        <span class="tuning-gains">Kp=${m.data.kp}<br>Ki=${m.data.ki} Kd=${m.data.kd}</span>
      `;
      btn.addEventListener('click', async () => {
        await this.api.applyTuning(m.key);
        // Preenche os ganhos do PAINEL correspondente — fluxo direto
        // sintonizar → testar na mesma aba.
        panel.inputs[0].value = m.data.kp;
        panel.inputs[1].value = m.data.ki;
        panel.inputs[2].value = m.data.kd;
        const alvo = panel.plant === 'position' ? 'posição' : 'velocidade';
        this._toast(`${m.name}: ganhos de ${alvo} preenchidos — toque em Iniciar para testar`, 'success');
      });
      suggestions.appendChild(btn);
    });
  }

  _onMotorCmd(direction) {
    const speed = parseInt(this.els.inputSpeed.value);
    this.api.motorCommand(direction, speed);
    this.running = true;
    this.paused = false;
    this._updateButtons();
    this._updateBadges();
    this.chart.clear();
    this.chartTime = 0;
  }

  _updateButtons() {
    const active = this.running && !this.paused;
    if (this.mode === 'speed') {
      this.els.btnRunSpeed.textContent = active ? 'Parar' : 'Iniciar';
      this.els.btnRunSpeed.classList.toggle('btn-success', !active);
      this.els.btnRunSpeed.classList.toggle('btn-danger', active);
    } else if (this.mode === 'position') {
      this.els.btnRunPos.textContent = active ? 'Parar' : 'Iniciar';
      this.els.btnRunPos.classList.toggle('btn-success', !active);
      this.els.btnRunPos.classList.toggle('btn-danger', active);
      // Zerar posição só faz sentido com o controle parado
      this.els.btnZero.disabled = this.running;
    }
  }

  /**
   * Badge "Rodando/Parado" em cada painel de modo.
   */
  _updateBadges() {
    const speedActive = this.running && this.mode === 'speed';
    const posActive = this.running && this.mode === 'position';
    const olActive = this.running && this.mode === 'open-loop';

    this.els.badgeSpeed.textContent = speedActive ? 'Rodando' : 'Parado';
    this.els.badgeSpeed.classList.toggle('running', speedActive);
    this.els.badgePosition.textContent = posActive ? 'Rodando' : 'Parado';
    this.els.badgePosition.classList.toggle('running', posActive);
    this.els.badgeOpenLoop.textContent = olActive ? 'Rodando' : 'Parado';
    this.els.badgeOpenLoop.classList.toggle('running', olActive);
  }

  _startPolling() {
    // Polling do encoder (100ms) — gráfico + leitura de medida/erro
    setInterval(async () => {
      const data = await this.api.getEncoderForChart();
      if (!data) return;

      this.chartTime += CONFIG.POLL_INTERVAL / 1000;

      const isPos = this.mode === 'position';
      const isSpeed = this.mode === 'speed';
      const value = isPos ? (data.angle ?? 0) : Math.abs(data.rpm ?? 0);

      // Setpoint no gráfico
      let showSetpoint = false;
      let setpoint = null;
      if (isPos) {
        showSetpoint = this.posConfigured;
        setpoint = this.api.posTarget;
      } else if (isSpeed) {
        showSetpoint = this.setpointConfigured;
        setpoint = this.api.setpoint;
      }

      this.chart.addPoint(this.chartTime, value, setpoint);

      // ---- Leitura ao vivo ----
      if (isSpeed) {
        const rpmAbs = Math.abs(data.rpm ?? 0);
        this.els.roRpm.textContent = rpmAbs.toFixed(0);
        if (this.setpointConfigured && setpoint !== null) {
          const err = setpoint - rpmAbs;
          this.els.roSp.textContent = setpoint.toFixed(0);
          this.els.roErr.textContent = (err >= 0 ? '+' : '') + err.toFixed(0);
          // Cor por magnitude relativa (±5% do setpoint = "ok")
          const rel = Math.abs(err) / Math.max(1, Math.abs(setpoint));
          this._setErrClass(this.els.roErr, rel <= 0.05 ? 'ok' : (err > 0 ? 'pos' : 'neg'));
        }
        if (this._fwOutput !== null) {
          this.els.roOut.textContent = this._fwOutput.toFixed(0);
        }
        this.readoutSpeedIdle = false;
      } else if (isPos) {
        const angle = data.angle ?? 0;
        this.els.roAngle.textContent = angle.toFixed(1);
        if (this.posConfigured && setpoint !== null) {
          const err = setpoint - angle;
          this.els.roTarget.textContent = `${setpoint.toFixed(0)}°`;
          this.els.roPErr.textContent = `${err >= 0 ? '+' : ''}${err.toFixed(1)}°`;
          // "Na faixa" = dentro da deadband do firmware (±2°): verde.
          // Fora dela: abaixo do alvo âmbar / acima azul.
          this._setErrClass(this.els.roPErr, Math.abs(err) <= 2 ? 'ok' : (err > 0 ? 'pos' : 'neg'));
        }
        if (this._fwOutput !== null) {
          this.els.roPOut.textContent = this._fwOutput.toFixed(0);
        }
        this.readoutPosIdle = false;
      } else {
        this.els.roORpm.textContent = Math.abs(data.rpm ?? 0).toFixed(0);
      }

      // Feedback de sinal do encoder: taxa de pulsos (p/s) e alerta
      // quando o motor está acionado mas nenhum pulso chega.
      const nowMs = performance.now();
      if (this._lastPulses === null) {
        this._lastPulses = data.pulses;
        this._lastPulseMs = nowMs;
      } else {
        const dtS = (nowMs - this._lastPulseMs) / 1000;
        if (dtS >= 0.5) {
          const rate = (data.pulses - this._lastPulses) / dtS;
          this._lastPulses = data.pulses;
          this._lastPulseMs = nowMs;
          this.els.statEncRate.textContent = `${rate.toFixed(0)} p/s`;
          const noSignal = Math.abs(rate) < 1 && this.running;
          this.els.statEncRate.classList.toggle('warn', noSignal);
          this.els.statPulses.textContent = data.pulses;
        }
      }
    }, CONFIG.POLL_INTERVAL);

    // Polling da config de controle (1s) — estado + saída do controlador
    setInterval(async () => {
      if (this.mode === 'speed') {
        const cfg = await this.api.getPidConfig();
        if (!cfg) return;
        this._fwRunning = !!cfg.running || this.api.autotuneActive;
        this._fwOutput = cfg.output !== undefined ? cfg.output : null;
        if (!this.setpointConfigured && cfg.setpoint) {
          this.els.inputSetpoint.value = cfg.setpoint;
        }
      } else if (this.mode === 'position') {
        const cfg = await this.api.getPosConfig();
        if (!cfg) return;
        this._fwRunning = !!cfg.running;
        this._fwOutput = cfg.output !== undefined ? cfg.output : null;
        if (!this.posConfigured && cfg.target !== undefined) {
          this.els.inputAngle.value = cfg.target;
        }
      }
      this._updateBadges();
    }, CONFIG.LIVE_INTERVAL);

    // Polling do status do sistema (2s)
    setInterval(async () => {
      const status = await this.api.getStatus();
      if (!status) {
        this.els.connDot.className = 'conn-dot offline';
        this.els.connLabel.textContent = 'desconectado';
        return;
      }

      this.els.connDot.className = 'conn-dot online';
      this.els.connLabel.textContent = CONFIG.USE_MOCK ? 'simulador' : 'conectado';
      this.els.version.textContent = `v${status.version}`;

      this.els.statMode.textContent = MODE_NAMES[status.mode] || status.mode || '--';
      this.els.statClients.textContent =
        status.stations !== undefined ? status.stations : '--';
      this.els.statSsid.textContent = status.ssid || '--';
      this.els.statIp.textContent = status.ip || '--';
      this.els.statHeap.textContent = `${(status.free_heap / 1024).toFixed(1)} KB`;
      this.els.statUptime.textContent = this._formatUptime(status.uptime_ms);
    }, CONFIG.STATUS_INTERVAL);
  }

  _formatUptime(ms) {
    const s = Math.floor(ms / 1000);
    const h = Math.floor(s / 3600);
    const m = Math.floor((s % 3600) / 60);
    const sec = s % 60;
    return `${String(h).padStart(2, '0')}:${String(m).padStart(2, '0')}:${String(sec).padStart(2, '0')}`;
  }

  _toast(message, type = 'info') {
    const container = document.getElementById('toast-container');
    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.textContent = message;
    container.appendChild(toast);
    setTimeout(() => toast.remove(), 4000);
  }
}

// ============================================================
// INICIALIZAÇÃO
// ============================================================

document.addEventListener('DOMContentLoaded', () => {
  const simulator = new Simulator();
  const api = new APIClient(simulator);
  const chart = new ChartManager('chart');
  const ui = new UIController(api, chart);

  // Expor para debug no console
  window.girino = { simulator, api, chart, ui };
});
