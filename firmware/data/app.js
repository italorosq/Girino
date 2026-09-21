/**
 * Girino — Interface Web (app.js)
 *
 * Módulos:
 *   Simulator   — modelo mock do motor+encoder+PID para testes sem hardware
 *   APIClient   — cliente HTTP para os endpoints /api/*
 *   ChartManager — gerencia Chart.js com rolling window
 *   UIController — lógica da interface, event listeners, polling
 */

// ============================================================
// CONFIGURAÇÃO
// ============================================================

const CONFIG = {
  USE_MOCK: false,            // true = simulador, false = hardware real
  POLL_INTERVAL: 100,         // ms (encoder)
  STATUS_INTERVAL: 2000,      // ms (status do sistema)
  CHART_MAX_POINTS: 300,      // ~30s a 100ms
  CHART_UPDATE_INTERVAL: 100, // ms
};

// Nomes amigáveis dos modos do firmware (campo "mode" da /api/status)
const MODE_NAMES = {
  'open-loop': 'Malha Aberta',
  pid: 'PID',
  position: 'Posição',
  autotune: 'Auto-Tune',
};

// Explicação curta de cada modo (mostrada abaixo dos radios)
const MODE_DESCRIPTIONS = {
  'open-loop': 'PWM manual direto: o motor gira em velocidade fixa, sem realimentação.',
  pid: 'Malha fechada: regula a velocidade (RPM) no setpoint usando o PID.',
  position: 'Malha fechada: vai até o ângulo alvo e tenta mantê-lo.',
  autotune: 'Identifica a planta (relé) e sugere ganhos para o PID de velocidade.',
};

// ============================================================
// SIMULADOR MOCK
// ============================================================

class Simulator {
  constructor() {
    this.time = 0;
    this.dt = CONFIG.POLL_INTERVAL / 1000; // em segundos

    // Estado do motor
    this.rpm = 0;
    this.pulses = 0;
    this.pwmTarget = 0;     // PWM alvo (0-100)
    this.direction = 0;     // -1, 0, +1

    // Modelo do motor (first-order)
    this.tau = 0.4;         // constante de tempo (s)
    this.deadTime = 0.08;   // atraso morto (s)
    this.noiseLevel = 1.5;  // ruído em RPM
    this.maxRPM = 180;      // RPM a 100% PWM

    // Fila de atraso para dead time
    this.delayBuffer = [];
    this.delaySteps = Math.round(this.deadTime / this.dt);

    // PID
    this.pidEnabled = false;
    this.setpoint = 50;
    this.kp = 0;
    this.ki = 0;
    this.kd = 0;
    this.integral = 0;
    this.prevError = 0;
    this.pidOutput = 0;

    // Auto-tune
    this.autotuneRunning = false;
    this.autotuneRelayAmp = 20;
    this.autotuneCycles = 3;
    this.autotunePhase = 0;
    this.autotuneOscillations = [];
    this.autotuneLastCross = 0;
    this.autotuneCrossCount = 0;
    this.autotuneStartTime = 0;
    this.autotuneProgress = 0;
    this.autotuneResults = null;
    this.autotunePrevSign = 1;

    // Histórico para resposta ao degrau
    this.responseData = [];
    this.responseStartTime = 0;

    // Buffer de saída para simular atraso
    for (let i = 0; i < this.delaySteps; i++) {
      this.delayBuffer.push(0);
    }
  }

  noise() {
    return (Math.random() - 0.5) * 2 * this.noiseLevel;
  }

  update() {
    this.time += this.dt;
    const dt = this.dt;

    if (this.autotuneRunning) {
      this._updateAutotune(dt);
    } else if (this.pidEnabled) {
      this._updatePID(dt);
    } else {
      this._updateOpenLoop(dt);
    }

    // Atualizar pulsos (600 PPR)
    this.pulses += Math.round((this.rpm / 60) * 600 * dt);

    // Registrar dados para resposta ao degrau
    if (this.pidEnabled || this.autotuneRunning) {
      this.responseData.push([this.time - this.responseStartTime, this.setpoint, this.rpm]);
      if (this.responseData.length > 1000) {
        this.responseData.shift();
      }
    }
  }

  _updateOpenLoop(dt) {
    const targetRPM = (this.pwmTarget / 100) * this.maxRPM * this.direction;
    this.delayBuffer.push(targetRPM);
    const delayedTarget = this.delayBuffer.shift();
    this.rpm += (delayedTarget - this.rpm) * (dt / this.tau) + this.noise() * dt * 10;
    this.rpm = Math.max(-this.maxRPM, Math.min(this.maxRPM, this.rpm));
  }

  _updatePID(dt) {
    const error = this.setpoint - this.rpm;

    // Anti-windup
    this.integral += error * dt;
    const integralMax = 100;
    if (this.integral > integralMax) this.integral = integralMax;
    if (this.integral < -integralMax) this.integral = -integralMax;

    // Derivative on measurement (evita derivative kick)
    const derivative = -(this.rpm - (this.prevRpm || this.rpm)) / dt;

    // PID output
    this.pidOutput = this.kp * error + this.ki * this.integral + this.kd * derivative;

    // Saturar saída
    let output = Math.max(-100, Math.min(100, this.pidOutput));

    // Simular motor com resposta de primeira ordem
    const targetRPM = (output / 100) * this.maxRPM;
    this.delayBuffer.push(targetRPM);
    const delayedTarget = this.delayBuffer.shift();
    this.rpm += (delayedTarget - this.rpm) * (dt / this.tau) + this.noise() * dt * 5;
    this.rpm = Math.max(0, Math.min(this.maxRPM, this.rpm));

    this.prevError = error;
    this.prevRpm = this.rpm;
  }

  _updateAutotune(dt) {
    const elapsed = this.time - this.autotuneStartTime;
    const totalDuration = this.autotuneCycles * 2 * 0.5; // ~0.5s por semiciclo estimado

    this.autotuneProgress = Math.min(100, (elapsed / totalDuration) * 100);

    // Relay feedback: alternar entre +amplitude e -amplitude baseado no sinal do erro
    const error = this.setpoint - this.rpm;
    const sign = error > 0 ? 1 : -1;

    // Detectar cruzamento por zero (mudança de sinal do erro)
    if (sign !== this.autotunePrevSign) {
      this.autotuneCrossCount++;
      if (this.autotuneCrossCount >= 2) {
        const halfPeriod = this.time - this.autotuneLastCross;
        this.autotuneOscillations.push(halfPeriod * 2); // período completo
      }
      this.autotuneLastCross = this.time;
      this.autotunePrevSign = sign;
    }

    // Aplicar relay
    const relayOutput = sign * this.autotuneRelayAmp;

    // Simular motor
    const targetRPM = (relayOutput / 100) * this.maxRPM + this.setpoint;
    this.delayBuffer.push(targetRPM);
    const delayedTarget = this.delayBuffer.shift();
    this.rpm += (delayedTarget - this.rpm) * (dt / this.tau) + this.noise() * dt * 5;
    this.rpm = Math.max(0, Math.min(this.maxRPM, this.rpm));

    // Verificar se terminou
    if (this.autotuneCrossCount >= this.autotuneCycles * 2 + 2) {
      this._finishAutotune();
    }

    // Timeout de segurança (15s)
    if (elapsed > 15) {
      this._finishAutotune();
    }
  }

  _finishAutotune() {
    this.autotuneRunning = false;

    // Calcular Tu (período médio das oscilações)
    let tu = 0;
    if (this.autotuneOscillations.length > 0) {
      tu = this.autotuneOscillations.reduce((a, b) => a + b, 0) / this.autotuneOscillations.length;
    } else {
      tu = 0.5; // fallback
    }

    // Calcular Ku (ganho crítico)
    // Ku = 4 * d / (pi * a)
    // d = amplitude do relay (em %), a = amplitude da oscilação (em RPM)
    const a = this.maxRPM * 0.15; // amplitude estimada da oscilação
    const ku = (4 * this.autotuneRelayAmp) / (Math.PI * (a / this.maxRPM) * 100);

    this.autotuneResults = {
      ku: parseFloat(ku.toFixed(2)),
      tu: parseFloat(tu.toFixed(3)),
      zn: this._calcZN(ku, tu),
      tl: this._calcTL(ku, tu),
      cc: this._calcCC(ku, tu),
    };

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

  startAutotune(relayAmp, cycles) {
    this.autotuneRunning = true;
    this.autotuneRelayAmp = relayAmp;
    this.autotuneCycles = cycles;
    this.autotuneStartTime = this.time;
    this.autotuneOscillations = [];
    this.autotuneCrossCount = 0;
    this.autotuneLastCross = this.time;
    this.autotuneProgress = 0;
    this.autotuneResults = null;
    this.autotunePrevSign = 1;
    this.responseData = [];
    this.responseStartTime = this.time;
    this.rpm = this.setpoint * 0.5; // começar abaixo do setpoint
  }

  startPID() {
    this.pidEnabled = true;
    this.autotuneRunning = false;
    this.integral = 0;
    this.prevError = 0;
    this.prevRpm = this.rpm;
    this.responseData = [];
    this.responseStartTime = this.time;
  }

  stopPID() {
    this.pidEnabled = false;
    this.pidOutput = 0;
  }

  getEncoder() {
    return {
      pulses: this.pulses,
      rpm: parseFloat(this.rpm.toFixed(1)),
    };
  }

  getPidConfig() {
    return {
      kp: this.kp,
      ki: this.ki,
      kd: this.kd,
      setpoint: this.setpoint,
      mode: this.pidEnabled ? 'pid' : (this.autotuneRunning ? 'autotune' : 'open-loop'),
    };
  }

  getStatus() {
    return {
      version: '0.1.0-mock',
      wifi_rssi: -42,
      uptime_ms: Math.round(this.time * 1000),
      free_heap: 32768,
    };
  }

  getAutotuneStatus() {
    return {
      status: this.autotuneRunning ? 'running' : (this.autotuneResults ? 'done' : 'idle'),
      progress: this.autotuneProgress,
      results: this.autotuneResults,
    };
  }

  getTuning() {
    if (!this.autotuneResults) return null;
    return {
      ku: this.autotuneResults.ku,
      tu: this.autotuneResults.tu,
      ZN: this.autotuneResults.zn,
      TL: this.autotuneResults.tl,
      CC: this.autotuneResults.cc,
    };
  }

  getPidResponse() {
    return {
      data: this.responseData.slice(-CONFIG.CHART_MAX_POINTS),
      setpoint: this.setpoint,
    };
  }

  motorCommand(direction, speed) {
    this.direction = direction === 'forward' ? 1 : (direction === 'reverse' ? -1 : 0);
    this.pwmTarget = speed;
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
    this.setpoint = 50;
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
      return { ok: true };
    }
    const res = await this._fetch('/api/pid/start', { method: 'POST' });
    if (res) {
      this.pidActive = true;
      this.autotuneActive = false;
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
      this.simulator.kp = kp;
      this.simulator.ki = ki;
      this.simulator.kd = kd;
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
      this.posActive = false;
      return { ok: true };
    }
    const res = await this._fetch('/api/position/stop', { method: 'POST' });
    if (res) this.posActive = false;
    return res;
  }

  async zeroPos() {
    if (CONFIG.USE_MOCK) return { ok: true };
    const res = await this._fetch('/api/position/zero', { method: 'POST' });
    if (res) this.posActive = false;
    return res;
  }

  async startAutotune(relayAmplitude, cycles, setpoint, bias) {
    if (setpoint !== undefined) this.setpoint = setpoint;
    if (CONFIG.USE_MOCK) {
      this.simulator.startAutotune(relayAmplitude, cycles);
      this.autotuneActive = true;
      this.pidActive = false;
      return { ok: true };
    }
    const res = await this._fetch('/api/pid/autotune', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: `relay_amplitude=${relayAmplitude}&cycles=${cycles}&setpoint=${this.setpoint}&bias=${bias}`,
    });
    if (res) {
      this.autotuneActive = true;
      this.pidActive = false;
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
      return { ok: true };
    }
    // Comando manual desliga PID/autotune no firmware
    if (direction === 'stop') {
      this.pidActive = false;
      this.autotuneActive = false;
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
    this.dataPoints = { time: [], setpoint: [], rpm: [] };
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
            label: 'Posição Real (RPM)',
            data: [],
            borderColor: '#4caf50',
            backgroundColor: 'rgba(76, 175, 80, 0.1)',
            borderWidth: 2,
            pointRadius: 0,
            tension: 0.3,
            fill: true,
          },
          {
            label: 'Setpoint (RPM)',
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
              label: (item) => `${item.dataset.label}: ${item.formattedValue} RPM`,
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

  addPoint(time, rpm, setpoint) {
    const maxPoints = CONFIG.CHART_MAX_POINTS;

    this.dataPoints.time.push(time.toFixed(1));
    this.dataPoints.rpm.push(rpm);
    this.dataPoints.setpoint.push(setpoint);

    if (this.dataPoints.time.length > maxPoints) {
      this.dataPoints.time.shift();
      this.dataPoints.rpm.shift();
      this.dataPoints.setpoint.shift();
    }

    this.chart.data.labels = this.dataPoints.time;
    this.chart.data.datasets[0].data = this.dataPoints.rpm;
    this.chart.data.datasets[1].data = this.dataPoints.setpoint;
    this.chart.update('none');
  }

  clear() {
    this.dataPoints = { time: [], setpoint: [], rpm: [] };
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
    this.chart.data.datasets[0].label = isDeg ? 'Ângulo Real (°)' : 'Posição Real (RPM)';
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

  exportCSV() {
    let csv = 'Tempo (s),Setpoint (RPM),Posicao Real (RPM)\n';
    for (let i = 0; i < this.dataPoints.time.length; i++) {
      csv += `${this.dataPoints.time[i]},${this.dataPoints.setpoint[i]},${this.dataPoints.rpm[i]}\n`;
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
    this.mode = 'open-loop';
    this.running = false;
    this.paused = false;
    this.chartTime = 0;
    this.setpointConfigured = false;
    this.posConfigured = false;
    this._autotunePoll = null;
    this._lastPulses = null;
    this._lastPulseMs = 0;

    this._cacheElements();
    this._bindEvents();
    this._setMode('open-loop');
    this._startPolling();
  }

  _cacheElements() {
    this.els = {
      // Header
      version: document.getElementById('version'),
      connDot: document.getElementById('conn-dot'),
      connLabel: document.getElementById('conn-label'),

      // Controles
      btnStart: document.getElementById('btn-start'),
      btnPause: document.getElementById('btn-pause'),
      btnStop: document.getElementById('btn-stop'),

      // PID
      inputSetpoint: document.getElementById('input-setpoint'),
      selectTuning: document.getElementById('select-tuning'),
      inputKp: document.getElementById('input-kp'),
      inputKi: document.getElementById('input-ki'),
      inputKd: document.getElementById('input-kd'),
      btnApplyPid: document.getElementById('btn-apply-pid'),
      btnRunPid: document.getElementById('btn-run-pid'),
      pidGains: document.getElementById('pid-gains'),

      // Posição (ângulo)
      inputAngle: document.getElementById('input-angle'),
      inputPkp: document.getElementById('input-pkp'),
      inputPki: document.getElementById('input-pki'),
      inputPkd: document.getElementById('input-pkd'),
      btnApplyPos: document.getElementById('btn-apply-pos'),
      btnRunPos: document.getElementById('btn-run-pos'),
      btnZero: document.getElementById('btn-zero'),

      // Descrição do modo
      modeDesc: document.getElementById('mode-desc'),

      // Auto-tune
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

      // Malha aberta
      inputSpeed: document.getElementById('input-speed'),
      speedDisplay: document.getElementById('speed-display'),
      btnForward: document.getElementById('btn-forward'),
      btnReverse: document.getElementById('btn-reverse'),

      // Status
      statRpm: document.getElementById('stat-rpm'),
      statAngle: document.getElementById('stat-angle'),
      statSetpoint: document.getElementById('stat-setpoint'),
      setpointStat: document.getElementById('setpoint-stat'),
      statPulses: document.getElementById('stat-pulses'),
      statMode: document.getElementById('stat-mode'),
      statClients: document.getElementById('stat-clients'),
      statEncRate: document.getElementById('stat-enc-rate'),
      statSsid: document.getElementById('stat-ssid'),
      statIp: document.getElementById('stat-ip'),
      statHeap: document.getElementById('stat-heap'),
      statUptime: document.getElementById('stat-uptime'),

      // Chart
      btnExportCsv: document.getElementById('btn-export-csv'),
      btnExportExp: document.getElementById('btn-export-exp'),

      // Panels
      modePanel: document.getElementById('mode-panel'),
      pidContent: document.getElementById('pid-content'),
      positionContent: document.getElementById('position-content'),
      autotuneContent: document.getElementById('autotune-content'),
      openLoopContent: document.getElementById('open-loop-content'),
      setpointContent: document.getElementById('setpoint-content'),
    };
  }

  _bindEvents() {
    // Modo
    document.querySelectorAll('input[name="mode"]').forEach((radio) => {
      radio.addEventListener('change', (e) => this._setMode(e.target.value));
    });

    // Controles
    this.els.btnStart.addEventListener('click', () => this._onStart());
    this.els.btnPause.addEventListener('click', () => this._onPause());
    this.els.btnStop.addEventListener('click', () => this._onStop());

    // PID
    this.els.btnApplyPid.addEventListener('click', () => this._onApplyPid());
    this.els.selectTuning.addEventListener('change', () => this._onTuningChange());

    // Posição (ângulo)
    this.els.btnApplyPos.addEventListener('click', () => this._onApplyPos());
    this.els.btnZero.addEventListener('click', () => this._onZero());

    // Botões Iniciar/Parar dentro dos painéis (PID e Posição)
    this.els.btnRunPid.addEventListener('click', () => this._onRunToggle());
    this.els.btnRunPos.addEventListener('click', () => this._onRunToggle());

    // Auto-tune
    this.els.btnAutotune.addEventListener('click', () => this._onAutotune());

    // Malha aberta
    this.els.inputSpeed.addEventListener('input', (e) => {
      this.els.speedDisplay.textContent = e.target.value;
    });
    this.els.btnForward.addEventListener('click', () => this._onMotorCmd('forward'));
    this.els.btnReverse.addEventListener('click', () => this._onMotorCmd('reverse'));

    // Export CSV
    this.els.btnExportCsv.addEventListener('click', () => {
      this.chart.exportCSV();
      this._toast('Dados exportados como CSV', 'success');
    });
    this.els.btnExportExp.addEventListener('click', () => this._onExportExperiment());
  }

  _setMode(mode) {
    // Trocar de modo cancela qualquer experimento em andamento no firmware
    if (this.mode !== mode &&
        (this.running || this.api.pidActive || this.api.autotuneActive || this.api.posActive)) {
      this._cancelExperiment();
    }

    this.mode = mode;

    // Esconder todos os conteúdos de modo
    this.els.openLoopContent.classList.add('hidden');
    this.els.setpointContent.classList.add('hidden');
    this.els.pidContent.classList.add('hidden');
    this.els.positionContent.classList.add('hidden');
    this.els.autotuneContent.classList.add('hidden');

    // Mostrar conteúdo correto e configurar o gráfico (RPM ou graus)
    switch (mode) {
      case 'open-loop':
        this.els.openLoopContent.classList.remove('hidden');
        this.chart.setUnits('rpm');
        this.chart.setSetpointVisible(false);
        this.els.setpointStat.classList.add('hidden');
        this.setpointConfigured = false;
        this.els.btnStart.disabled = false;
        break;
      case 'pid':
        this.els.setpointContent.classList.remove('hidden');
        this.els.pidContent.classList.remove('hidden');
        this.chart.setUnits('rpm');
        this.chart.setSetpointVisible(true);
        this.els.setpointStat.classList.remove('hidden');
        this.els.btnStart.disabled = false;
        break;
      case 'position':
        this.els.positionContent.classList.remove('hidden');
        this.chart.setUnits('deg');
        this.chart.setSetpointVisible(true);
        this.els.setpointStat.classList.remove('hidden');
        this.els.btnStart.disabled = false;
        break;
      case 'autotune':
        this.els.setpointContent.classList.remove('hidden');
        this.els.autotuneContent.classList.remove('hidden');
        this.chart.setUnits('rpm');
        this.chart.setSetpointVisible(true);
        this.els.setpointStat.classList.remove('hidden');
        this.els.btnStart.disabled = true;
        break;
    }

    // Reinicia o gráfico ao trocar de grandeza/modo
    this.chart.clear();
    this.chartTime = 0;
    this._updateButtons();

    // Descrição do modo selecionado
    this.els.modeDesc.textContent = MODE_DESCRIPTIONS[mode] || '';

    // Atualizar radio visual
    document.querySelectorAll('input[name="mode"]').forEach((r) => {
      r.checked = r.value === mode;
    });
  }

  /**
   * Cancela o experimento em andamento no firmware (PID, posição ou
   * auto-tune). Usado ao trocar de modo ou pelo botão Parar.
   */
  _cancelExperiment() {
    if (this._autotunePoll) {
      clearInterval(this._autotunePoll);
      this._autotunePoll = null;
    }
    this.api.pidActive = false;
    this.api.autotuneActive = false;
    this.api.posActive = false;
    this.els.btnAutotune.disabled = false;
    this.els.progressContainer.classList.add('hidden');
    this.api.motorCommand('stop', 0); // firmware cancela todos os modos
    this.running = false;
    this.paused = false;
    this._updateButtons();
  }

  /**
   * Alterna Iniciar/Parar pelo botão dentro do painel do modo ativo.
   */
  _onRunToggle() {
    if (this.running) {
      this._onStop();
    } else {
      this._onStart();
    }
  }

  async _onStart() {
    if (this.mode === 'pid') {
      await this._onApplyPid();
      await this.api.startPid();
      this.running = true;
      this.paused = false;
      this._toast('PID iniciado', 'success');
    } else if (this.mode === 'position') {
      await this._onApplyPos();
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
      this._toast('Motor ligado', 'success');
    }
    this._updateButtons();
    this.chart.clear();
    this.chartTime = 0;
  }

  async _onPause() {
    if (this.mode === 'pid') {
      if (this.paused) {
        await this.api.startPid();
        this._toast('PID retomado', 'info');
      } else {
        await this.api.stopPid();
        this._toast('PID pausado', 'warning');
      }
    } else if (this.mode === 'position') {
      if (this.paused) {
        await this.api.startPos(this.api.posTarget);
        this._toast('Posição retomada', 'info');
      } else {
        await this.api.stopPos();
        this.api.motorCommand('stop', 0);
        this._toast('Posição pausada', 'warning');
      }
    } else {
      if (this.paused) {
        const speed = parseInt(this.els.inputSpeed.value);
        this.api.motorCommand('forward', speed);
        this._toast('Motor retomado', 'info');
      } else {
        this.api.motorCommand('stop', 0);
        this._toast('Motor pausado', 'warning');
      }
    }
    this.paused = !this.paused;
    this._updateButtons();
  }

  async _onStop() {
    if (this.mode === 'pid') {
      await this.api.stopPid();
    } else if (this.mode === 'position') {
      await this.api.stopPos();
    }
    this._cancelExperiment();
    this._toast('Parado', 'info');
  }

  async _onApplyPid() {
    const kp = parseFloat(this.els.inputKp.value) || 0;
    const ki = parseFloat(this.els.inputKi.value) || 0;
    const kd = parseFloat(this.els.inputKd.value) || 0;
    const setpoint = parseFloat(this.els.inputSetpoint.value) || 1000;
    if (kp === 0 && ki === 0 && kd === 0) {
      this._toast('Aviso: ganhos zerados — o motor não vai se mover', 'warning');
    }
    await this.api.setPidConfig(kp, ki, kd, setpoint);
    this.setpointConfigured = true;
    this.els.statSetpoint.textContent = setpoint.toFixed(0);
    this._toast(`PID configurado: Kp=${kp} Ki=${ki} Kd=${kd} SP=${setpoint}`, 'info');
  }

  async _onApplyPos() {
    const kp = parseFloat(this.els.inputPkp.value) || 0;
    const ki = parseFloat(this.els.inputPki.value) || 0;
    const kd = parseFloat(this.els.inputPkd.value) || 0;
    const target = parseFloat(this.els.inputAngle.value) || 0;
    await this.api.setPosConfig(kp, ki, kd, target);
    this.posConfigured = true;
    this.els.statSetpoint.textContent = `${target.toFixed(0)}°`;
    this._toast(`Posição: Kp=${kp} Ki=${ki} Kd=${kd} Alvo=${target}°`, 'info');
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
    this.els.statAngle.textContent = '0.0°';
    this._updateButtons();
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

  _onTuningChange() {
    const method = this.els.selectTuning.value;
    const isManual = method === 'manual';
    this.els.inputKp.disabled = !isManual;
    this.els.inputKi.disabled = !isManual;
    this.els.inputKd.disabled = !isManual;

    if (!isManual) {
      // Preencher ganhos com a sugestão do auto-tune (firmware ou simulador).
      // O firmware responde em minúsculas (zn/tl/cc); aceitamos ambos.
      this.api.getTuning().then((tuning) => {
        if (!tuning) return;
        const gains = tuning[method] || tuning[method.toUpperCase()];
        if (gains) {
          this.els.inputKp.value = gains.kp;
          this.els.inputKi.value = gains.ki;
          this.els.inputKd.value = gains.kd;
        }
      });
    }
  }

  async _onAutotune() {
    const relayAmp = parseInt(this.els.inputRelay.value) || 18;
    const bias = parseInt(this.els.inputBias.value) || 82;
    const cycles = parseInt(this.els.inputCycles.value) || 3;
    const setpoint = parseFloat(this.els.inputSetpoint.value) || 1000;

    this.els.btnAutotune.disabled = true;
    this.els.progressContainer.classList.remove('hidden');
    this.els.autotuneResults.classList.add('hidden');
    this.els.progressBar.style.width = '0%';
    this.els.progressLabel.textContent = '0%';

    const res = await this.api.startAutotune(relayAmp, cycles, setpoint, bias);
    if (!res) {
      this.els.btnAutotune.disabled = false;
      this.els.progressContainer.classList.add('hidden');
      this._toast('Falha ao iniciar Auto-Tune', 'error');
      return;
    }

    this.running = true;
    this.paused = false;
    this._updateButtons();
    this._toast('Auto-Tune iniciado...', 'info');
    this.chart.clear();
    this.chartTime = 0;

    // Polling do status do auto-tune (200 ms)
    this._autotunePoll = setInterval(async () => {
      const status = await this.api.getAutotuneStatus();
      if (!status) return; // sem resposta agora; tenta no próximo ciclo

      this.els.progressBar.style.width = `${status.progress}%`;
      this.els.progressLabel.textContent = `${Math.round(status.progress)}%`;

      if (status.status === 'done' && status.results) {
        clearInterval(this._autotunePoll);
        this._autotunePoll = null;
        this.running = false;
        this.els.btnAutotune.disabled = false;
        this._updateButtons();
        this._showAutotuneResults(status.results);
        this._toast('Auto-Tune concluído!', 'success');
      } else if (status.status === 'failed') {
        clearInterval(this._autotunePoll);
        this._autotunePoll = null;
        this.running = false;
        this.els.btnAutotune.disabled = false;
        this.els.progressContainer.classList.add('hidden');
        this._updateButtons();
        this._toast('Auto-Tune falhou: oscilação insuficiente', 'error');
      }
    }, 200);
  }

  _showAutotuneResults(results) {
    this.els.autotuneResults.classList.remove('hidden');
    this.els.valKu.textContent = results.ku;
    this.els.valTu.textContent = `${results.tu}s`;

    // Gerar botões de sugestão
    const suggestions = this.els.tuningSuggestions;
    suggestions.innerHTML = '';

    const methods = [
      { key: 'zn', name: 'Ziegler-Nichols', data: results.zn },
      { key: 'tl', name: 'Tyreus-Luyben', data: results.tl },
      { key: 'cc', name: 'Cohen-Coon', data: results.cc },
    ];

    methods.forEach((m) => {
      const btn = document.createElement('button');
      btn.className = 'tuning-btn';
      btn.innerHTML = `
        <span class="tuning-name">${m.name}</span>
        <span class="tuning-gains">Kp=${m.data.kp} Ki=${m.data.ki} Kd=${m.data.kd}</span>
      `;
      btn.addEventListener('click', async () => {
        await this.api.applyTuning(m.key);
        this.els.inputKp.value = m.data.kp;
        this.els.inputKi.value = m.data.ki;
        this.els.inputKd.value = m.data.kd;
        this.els.selectTuning.value = m.key;
        this._onTuningChange();
        this._toast(`${m.name} aplicado`, 'success');
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
    this.chart.clear();
    this.chartTime = 0;
  }

  _updateButtons() {
    this.els.btnStart.disabled =
      (this.running && !this.paused) || this.mode === 'autotune';
    this.els.btnPause.disabled = !this.running || this.mode === 'autotune';
    this.els.btnStop.disabled = !this.running;

    // Botões Iniciar/Parar dentro dos painéis ficam em sincronia
    const pidStopped = !(this.mode === 'pid' && this.running);
    const posStopped = !(this.mode === 'position' && this.running);
    this.els.btnRunPid.textContent = pidStopped ? 'Iniciar' : 'Parar';
    this.els.btnRunPos.textContent = posStopped ? 'Iniciar' : 'Parar';
    this.els.btnRunPid.classList.toggle('btn-success', pidStopped);
    this.els.btnRunPid.classList.toggle('btn-danger', !pidStopped);
    this.els.btnRunPos.classList.toggle('btn-success', posStopped);
    this.els.btnRunPos.classList.toggle('btn-danger', !posStopped);

    // Zerar posição só faz sentido com o controle parado
    this.els.btnZero.disabled = this.mode === 'position' && this.running;
  }

  _startPolling() {
    // Polling do encoder (100ms)
    setInterval(async () => {
      const data = await this.api.getEncoderForChart();
      if (!data) return;

      this.chartTime += CONFIG.POLL_INTERVAL / 1000;

      const isPos = this.mode === 'position';
      const value = isPos ? (data.angle ?? 0) : data.rpm;

      let showSetpoint = false;
      let setpoint = null;
      if (isPos) {
        showSetpoint = this.posConfigured;
        setpoint = this.api.posTarget;
      } else if (this.mode === 'pid' || this.mode === 'autotune') {
        showSetpoint = this.setpointConfigured;
        setpoint = this.api.setpoint;
      }

      this.chart.addPoint(this.chartTime, value, setpoint);

      // Atualizar status na tela
      this.els.statRpm.textContent = data.rpm.toFixed(1);
      this.els.statAngle.textContent = `${(data.angle ?? 0).toFixed(1)}°`;
      this.els.statSetpoint.textContent = showSetpoint
        ? (isPos ? `${setpoint.toFixed(0)}°` : setpoint.toFixed(0))
        : '--';
      this.els.statPulses.textContent = data.pulses;

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
        }
      }
    }, CONFIG.POLL_INTERVAL);

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
    setTimeout(() => toast.remove(), 3000);
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
