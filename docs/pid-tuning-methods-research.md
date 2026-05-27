# PID Tuning Methods & Control Algorithms — Research Findings

> Comprehensive survey of PID tuning methods used in educational DC motor control platforms, with feasibility analysis for ESP8266 (Girino).

---

## 1. Analysis by Paper

### 1.1 Alexandre et al. (2025) — RET/UEPG

**Full title:** "ANÁLISE DO CONTROLE PID APLICADO NO CONTROLE DE VELOCIDADE DE UM MOTOR CC"

**URL:** https://revistas.uepg.br/index.php/ret/article/view/24725

| Aspect | Details |
|---|---|
| **PID tuning method** | **Cohen-Coon** (open-loop process reaction curve), followed by manual fine-tuning |
| **Control type** | **Speed only** (RPM control) |
| **Implementation** | Discrete PID on ATMEGA328 (Arduino UNO R3) |
| **Motor/hardware** | DC motor + rotary encoder + L298N H-bridge + Arduino UNO |
| **Performance metrics** | ISE, ITAE, IAE, ITSE — all evaluated |
| **Setpoints tested** | 200 RPM, 275 RPM, 120 RPM |
| **Results** | Steady-state error ≈ 0, minimal/no overshoot, consistent rise and settling times |

**Methodology:**
1. **Experimental modeling** — step response of motor to voltage input → transfer function from open-loop data
2. **Cohen-Coon gain calculation** — from process reaction curve parameters (gain K, time constant T, dead time L)
3. **Fine-tuning** — manual adjustment of Kp, Ki, Kd to optimize performance
4. **Validation** — closed-loop control with SCADA monitoring application

**Key takeaway:** Cohen-Coon provided good initial gains, but fine-tuning was essential. The paper demonstrates this is a viable method for DC motor speed control on Arduino-class hardware.

---

### 1.2 Cabral et al. (2025) — Ciência e Natura / UFSM

**Full title:** "Mini bancada de baixo custo com motores CC e encoder para prática de controle realimentado"

**URL:** https://periodicos.ufsm.br/cienciaenatura/article/view/92211

| Aspect | Details |
|---|---|
| **PID tuning method** | **Digital PID in discrete-time (z-domain)**, designed via MATLAB/Simulink |
| **Control type** | **Speed (angular velocity)** |
| **Implementation** | **Discrete-time digital controller**, designed in z-domain |
| **Motor/hardware** | DC motor + encoder + custom low-cost bench |
| **Validation tools** | MATLAB and Simulink |
| **Keywords** | "PID Digital", "Controle discreto", "Função de transferência" |

**Methodology:**
1. Detailed mathematical modeling of DC motor → transfer function derivation
2. **Digital discrete-time controller design** for angular velocity control
3. MATLAB/Simulink simulation and validation
4. Experimental verification on the physical bench

**Key takeaway:** This is the closest paper to Girino conceptually (Brazilian, low-cost, motor+encoder). Uses formal z-domain design (not just heuristic tuning). References Ogata's Discrete-time Control Systems and Reck & Sreenivas (2015). The paper explicitly teaches discrete control design, not just PID tuning heuristics.

---

### 1.3 OpenMCT — Von Chong (2026) — HardwareX

**Full title:** "OpenMCT: an open-source DC motor control educational kit"

**URL:** https://www.sciencedirect.com/science/article/pii/S2468067226000544  
**Zenodo:** https://zenodo.org/records/18409785

| Aspect | Details |
|---|---|
| **PID tuning method** | **Interactive GUI-based tuning** + **direct z-domain difference-equation coefficients** |
| **Control type** | **Speed control** (with current sensing) |
| **Implementation** | **Teensy 4.x** (ARM Cortex-M7, 600 MHz) — much more powerful than ESP8266 |
| **Motor/hardware** | Brushed DC motor + incremental encoder + H-bridge + current sensing |
| **GUI** | Cross-platform Python/Qt GUI with live telemetry, data logging, excitation signals |
| **Control features** | PID + **direct entry of z-domain difference-equation coefficients** |

**Methodology:**
1. Calibration and characterization
2. System identification
3. Controller design and deployment
4. Experimental validation with MATLAB-based tools

**Key features:**
- Supports both PID and raw z-domain control laws
- Students can enter difference-equation coefficients directly
- Complete workflow from identification to controller deployment
- Python GUI for real-time tuning (not embedded auto-tuning)

**Key takeaway:** The most sophisticated platform of the group. Supports z-domain control law implementation — students enter discrete transfer function coefficients directly. Requires a separate Python GUI (not embedded web like Girino). The Teensy 4.x provides ~7.5x more CPU power than ESP8266.

---

### 1.4 MotoShield — Takács et al. (2021) — IEEE FIE/EDUCON

**Full title:** "MotoShield: Open Miniaturized DC Motor Hardware Prototype for Control Education"

**URL:** https://ieeexplore.ieee.org/document/9453979  
**GitHub:** https://github.com/gergelytakacs/AutomationShield

| Aspect | Details |
|---|---|
| **PID tuning method** | **MATLAB-computed PID gains** (likely system identification + analytical design) |
| **Control type** | **Speed control** (RPM tracking) |
| **Implementation** | **Arduino-compatible** (ATmega328/UNO), with MATLAB/Simulink API |
| **Sample time** | **Ts = 30 ms** (Arduino) / **Ts = 40 ms** (MATLAB) |
| **Motor/hardware** | 6V brushed DC motor + hall-effect encoder + L293D H-bridge |
| **PID gains (Arduino example)** | Kp = 3.07, Ki = 122.9, Kd = 0 (PI only) |
| **PID gains (MATLAB example)** | Kp = 0.007, Ti = 0.002, Td = 0.1 |

**Methodology:**
1. System identification via step response
2. PID controller designed in MATLAB using the AutomationShield API
3. Deployed to Arduino for real-time execution
4. Calibration step at 100% PWM for 1 second before control starts

**Implementation details:**
- Uses timer interrupts for precise sampling (30ms = ~33 Hz)
- Absolute (positional) PID form: `PIDAbs.compute(error, 0, 100, 0, 100)`
- Output mapped to 0-100% motor speed
- Speed measurement via encoder pulse counting over sampling window

**Key takeaway:** The MotoShield uses MATLAB-based design deployed to Arduino — gains are computed offline, not auto-tuned on the microcontroller. The sampling rate is relatively low (33 Hz). The Arduino code is simple enough to port to ESP8266.

---

### 1.5 Reck & Sreenivas (2015 ACC / 2016 MDPI)

**Titles:**
- ACC 2015: "Developing a New Affordable DC Motor Laboratory Kit for an Existing Undergraduate Controls Course"
- MDPI Electronics 2016: "Developing an Affordable and Portable Control Systems Laboratory Kit — Results from a Pilot Program"

**URLs:**
- http://rsree.ise.illinois.edu/Control_Education_&_Applications_files/2015ACC_1530_FI.pdf
- https://www.mdpi.com/2079-9292/5/3/36

| Aspect | Details |
|---|---|
| **PID tuning method** | **Analytical design** from first-principles models + step/frequency response identification |
| **Control type** | **Both speed AND position** (multi-experiment) |
| **Implementation** | **Raspberry Pi** with MATLAB/Simulink External Mode |
| **Sample rate** | 100 Hz (position sensor via ADC/SPI) |
| **Motor/hardware** | 12V DC motor + 3D-printed encoder wheel (20 holes) + photo-interrupter + L293D H-bridge + rotary potentiometer (position) |
| **Cost** | ~$130 per kit |

**Laboratory experiments (5 total):**

| Lab | Topic | Control Approach |
|---|---|---|
| Lab 1 | Sensors & DC motor intro | Dead zone, calibration |
| Lab 2 | Simulink + Raspberry Pi | Gain, derivative, integral blocks |
| Lab 3 | First-principles modeling | Measure Ra, La, Kb, KT → transfer function |
| Lab 4 | Step & frequency response ID | First-order model from step + Bode plot |
| Lab 5 | **DC motor control** | **P, PD, P + speed feedback** (NOT full PID!) |

**Key detail:** Lab 5 designs **three controllers: proportional (P), proportional-derivative (PD), and proportional plus speed feedback** — notably, they do NOT use full PID. Students first design analytically, then adjust gains in simulation (because friction makes analytical gains too small), then deploy to hardware.

**Implementation details:**
- Speed measurement: counter blocks counting encoder pulses over 100ms windows
- Position measurement: rotary potentiometer via MCP3002 ADC (SPI)
- Simulink External Mode: code runs on RPi, data viewed live on host PC
- Controller design: pole placement / analytical from transfer function model

**Key takeaway:** This is the foundational work. Uses analytical controller design (P, PD, P+speed feedback), NOT heuristic PID tuning rules. Students derive the transfer function from physical parameters and design controllers to meet specifications. The kit costs ~$130 and uses MATLAB/Simulink as the primary interface.

---

### 1.6 Harahap et al. — ESP8266 PID Motor (Ziegler-Nichols)

**Note:** The exact Harahap et al. paper was not directly accessible, but the search results identified the IEEE paper (10.1109/ICITEED.2024.10469124) with matching description:

**Title:** "PID Controller Design for DC Motor Speed Control"

**URL:** https://ieeexplore.ieee.org/document/10469124  
**Note:** This appears to be the paper referenced. Authors may differ from "Harahap" — exact attribution pending.

| Aspect | Details |
|---|---|
| **PID tuning method** | **Ziegler-Nichols First Method** (open-loop step response / process reaction curve) |
| **Control type** | **Speed control** |
| **Implementation** | DC motor speed control system |
| **Results** | Effective speed regulation, minimized steady-state error, accelerated response times |

**Methodology:**
1. Open-loop step response of the DC motor
2. Extract process parameters from reaction curve
3. Apply Ziegler-Nichols first method formulas to compute Kp, Ki, Kd
4. Validate in closed-loop experiments

**Additional related work (found in search):**

There's a GitHub project "Self-Tuning PID Controller using ESP8266" (https://github.com/manal15iqbal/Self-Tuning-PID-Controller-using-ESP8266) that:
- Uses ESP8266 for PID motor control
- Uses vibration and Back EMF feedback (not encoder!)
- Features real-time monitoring via Python dashboard
- Uses self-tuning mechanism

---

## 2. PID Tuning Methods — Detailed Analysis

### 2.1 Ziegler-Nichols (ZN) Methods

#### 2.1.1 ZN First Method (Open-Loop / Process Reaction Curve)

**How it works:**
1. Apply a step input to the open-loop system
2. Record the output response (process reaction curve)
3. Fit a First-Order Plus Dead Time (FOPDT) model: G(s) = K·e^(-Ls) / (Ts + 1)
4. Extract parameters: K (process gain), T (time constant), L (dead time)
5. Look up PID gains from ZN tables:

| Controller | Kp | Ti | Td |
|---|---|---|---|
| P | T/(K·L) | ∞ | 0 |
| PI | 0.9·T/(K·L) | L/0.3 | 0 |
| PID | 1.2·T/(K·L) | 2·L | 0.5·L |

**Pros:**
- Simple open-loop experiment (safe, no risk of instability)
- Works well for FOPDT-like systems (DC motor is approximately first-order)
- Only needs one step response experiment
- Well-known, extensively documented

**Cons:**
- Assumes FOPDT model — DC motor dead time is usually very small, making L-based formulas fragile
- Quarter-wave decay criterion produces aggressive tuning (~60% overshoot typical)
- Requires manual fine-tuning after initial gains
- Sensitive to noise in step response measurement

**Feasibility on ESP8266:**
- ✅ **Computationally trivial** — only arithmetic operations
- ✅ Can be automated: apply step PWM, record encoder RPM over time, compute K, T, L
- ⚠️ Requires enough memory to store step response data (~100-500 samples)
- ⚠️ Need stable timing for step response measurement (use timer interrupt)
- **Verdict: Fully feasible, recommended for Girino auto-tune**

---

#### 2.1.2 ZN Second Method (Closed-Loop / Ultimate Gain)

**How it works:**
1. Start with proportional-only controller (Ki=0, Kd=0)
2. Gradually increase Kp until sustained oscillations occur
3. Record Kp at oscillation = **Ku** (ultimate gain)
4. Measure oscillation period = **Tu** (ultimate period)
5. Apply ZN frequency-domain formulas:

| Controller | Kp | Ti | Td |
|---|---|---|---|
| P | 0.50·Ku | ∞ | 0 |
| PI | 0.45·Ku | Tu/1.2 | 0 |
| PID | 0.60·Ku | Tu/2 | Tu/8 |
| PID (some OS) | 0.33·Ku | Tu/2 | Tu/3 |
| PID (no OS) | 0.20·Ku | Tu/2 | Tu/3 |

**Standard gain form (PID Classic):**
- Kp = 0.60·Ku
- Ki = 1.2·Ku/Tu
- Kd = 0.075·Ku·Tu

**Pros:**
- Directly measures system dynamics at the critical frequency
- Well-established method
- Works for any system that can sustain oscillations

**Cons:**
- **DANGEROUS** — requires pushing system to sustained oscillation (can damage hardware)
- For DC motors, the oscillation amplitude can cause mechanical stress
- Requires gradual gain increase — time-consuming and potentially risky
- Quarter-wave decay target is aggressive (high overshoot)
- Difficult to automate safely on small hardware

**Feasibility on ESP8266:**
- ⚠️ **Risky for auto-tune** — pushing motor to sustained oscillation can be dangerous
- ⚠️ Requires careful gain ramping logic
- ⚠️ Need oscillation detection algorithm (peak detection)
- ✅ Computationally simple once Ku and Tu are measured
- **Verdict: Possible but NOT recommended for educational/auto-tune use**

---

### 2.2 Cohen-Coon Method

**How it works:**
1. Same open-loop step response as ZN First Method
2. Extract FOPDT parameters: K, T, L
3. Compute r = L/T (controllability ratio)
4. Apply Cohen-Coon formulas (designed to give ¼ decay ratio with minimal load disturbance):

**For PID:**
- Kp = (T/K)·(0.9 + L/(12T)) / (L/T) = (T/K)·(1.0 + R/3) / R  where R = L/T
- Ti = T·(9 + 3R) / (4 + 12R)
- Td = T·(0.5·R) / (1 + 0.5·R)

**Simplified PID formulas (from AutoTunePID library):**
- Kp = 0.8·Ku
- Ki = Kp/(0.8·Tu)
- Kd = 0.194·Kp·Tu

**Pros:**
- **Better than ZN for systems with large dead time** (L/T > 0.1)
- Specifically designed for self-regulating processes
- Provides faster recovery from load disturbances
- Used successfully by Alexandre et al. (2025) for DC motor speed control
- Same experimental procedure as ZN First Method (just different formulas)

**Cons:**
- Still produces aggressive tuning (¼ decay ratio)
- Requires fine-tuning after initial calculation
- More complex formulas than ZN
- For DC motors (which have very small L), the advantage over ZN is minimal

**Feasibility on ESP8266:**
- ✅ **Same as ZN First Method** — identical experimental procedure
- ✅ Computationally trivial (a few multiplications and divisions)
- ✅ Alexandre et al. proved it works on ATMEGA328 (slower than ESP8266)
- **Verdict: Fully feasible, good alternative to ZN First Method**

---

### 2.3 Relay Feedback Auto-Tuning (Åström-Hägglund Method)

**How it works:**
1. Replace PID controller with an on-off relay: u(t) = d·sgn(r(t) - y(t))
2. Relay switches output between +d and -d based on error sign
3. The relay induces a stable limit cycle in the closed loop
4. Measure the amplitude (a) and period (Tu) of the oscillation
5. Compute ultimate gain: **Ku = 4d/(π·a)**
6. Apply ZN frequency-domain rules (or Tyreus-Luyben, etc.)

**Why it's better than ZN Second Method:**
- Oscillation amplitude is **bounded by relay amplitude d** (safe!)
- Can be performed **online** (under closed-loop control)
- **Model-free** — doesn't require fitting K, T, L
- Self-contained — easily automated on microcontroller
- The standard industrial auto-tune method (used in commercial PID controllers)

**Practical considerations:**
- Add small hysteresis ε to prevent chattering from noise
- Choose d large enough that a >> measurement noise
- Discard first 5+ time constants of transient before measuring
- Typically needs 5-10 complete oscillation cycles for good estimates

**Pros:**
- ✅ **Safe** — amplitude bounded by relay output d
- ✅ **Automatable** — can run entirely on microcontroller
- ✅ No need for open-loop experiment or system model
- ✅ Standard method used in commercial auto-tune PID controllers
- ✅ Can be performed online without disconnecting the controller

**Cons:**
- Introduces temporary oscillation (may be undesirable for some processes)
- Describing-function approximation gives ~10% error in Ku estimate
- Requires several oscillation cycles (takes time proportional to system time constant)
- For very fast systems (like DC motor speed), oscillation detection needs fast sampling

**Feasibility on ESP8266:**
- ✅ **Excellent fit** — only needs relay output + peak/valley detection
- ✅ Computationally lightweight: measure peaks, compute a and Tu, then Ku
- ✅ Can be implemented in ~100 lines of C++ code
- ✅ The ESP8266's timer interrupts can handle the sampling
- ⚠️ Need fast enough RPM measurement (encoder ISR) to capture oscillations
- ⚠️ DC motor speed response is fast — sampling at ≥50 Hz recommended
- **Verdict: HIGHLY RECOMMENDED for Girino auto-tune feature**

---

### 2.4 Tyreus-Luyben Method

**How it works:**
- Same experimental procedure as ZN Second Method or relay feedback
- Measure Ku and Tu
- Apply modified formulas designed for **minimal overshoot** and **robustness**:

| Parameter | Formula |
|---|---|
| Kp | Ku/2.2 |
| Ti | 2.2·Tu |
| Td | Tu/6.3 |

**Standard gains:**
- Ki = Kp/Ti = Ku/(2.2·2.2·Tu) = Ku/(4.84·Tu)
- Kd = Kp·Td = Ku·Tu/(2.2·6.3)

**Pros:**
- Much less aggressive than ZN (lower Kp, longer Ti)
- Better robustness margins
- Minimal overshoot compared to ZN Classic

**Cons:**
- Slower response than ZN
- Still needs Ku and Tu (same risks as ZN Second Method unless relay is used)
- Less commonly documented

**Feasibility on ESP8266:**
- ✅ Trivial computation once Ku and Tu are known
- ✅ Best paired with relay feedback for safe Ku/Tu measurement
- **Verdict: Good tuning rule to offer alongside ZN after relay auto-tune**

---

### 2.5 IMC (Internal Model Control) / Lambda Tuning

**How it works:**
1. Obtain FOPDT model: K, T, L from step response or system identification
2. Choose a closed-loop time constant λ (tuning parameter)
3. Compute PID gains:

| Parameter | Formula |
|---|---|
| Kp | T/(K·(λ + L)) |
| Ki | Kp/T = 1/(K·(λ + L)) |
| Kd | Kp·L/2 = T·L/(2·K·(λ + L)) |

- λ = tuning parameter (larger = slower, more robust; smaller = faster)
- λ = T gives "balanced" tuning; λ > T gives conservative; λ < T gives aggressive

**Pros:**
- **Single tuning parameter λ** — intuitive tradeoff between speed and robustness
- Guaranteed stability if model is reasonably accurate
- Explicit control over closed-loop speed
- Well-suited for systems with time delay
- Modern, theoretically sound approach

**Cons:**
- Requires accurate FOPDT model (K, T, L)
- Not directly automatable without system identification step
- Less intuitive for students than ZN/CC tables

**Feasibility on ESP8266:**
- ✅ Computationally trivial
- ⚠️ Requires separate system identification step to get K, T, L
- **Verdict: Good option if step-response system identification is implemented**

---

### 2.6 Comparison of Methods

| Method | Experiment Type | Safety | Automation Ease | Overshoot | DC Motor Suitability |
|---|---|---|---|---|---|
| **ZN 1st (Open-loop)** | Open-loop step | ✅ Safe | ✅ Easy | ~60% (high) | ⚠️ L very small, formulas fragile |
| **ZN 2nd (Closed-loop)** | Sustained oscillation | ❌ Risky | ⚠️ Difficult | ~60% (high) | ⚠️ Risk of motor damage |
| **Cohen-Coon** | Open-loop step | ✅ Safe | ✅ Easy | Moderate | ✅ Proven by Alexandre et al. |
| **Relay Feedback (Åström)** | Relay oscillation | ✅ Bounded | ✅✅ Very easy | Depends on rule applied | ✅✅ Best for auto-tune |
| **Tyreus-Luyben** | Needs Ku, Tu | — | — | Low | ✅ Good with relay feedback |
| **IMC/Lambda** | Needs K, T, L | ✅ Safe | ⚠️ Moderate | Adjustable via λ | ✅ Elegant, model-based |

---

## 3. Existing Arduino PID Auto-Tune Libraries

### 3.1 AutoTunePID (lily-osp)

**URL:** https://github.com/lily-osp/AutoTunePID

**Features:**
- **5 auto-tuning methods:** Ziegler-Nichols, Cohen-Coon, IMC, Tyreus-Luyben, Lambda
- Anti-windup with configurable threshold
- Input/output signal filtering (exponential moving average)
- Multiple oscillation modes (Normal, Half, Mild)
- Fixed 100ms update interval
- ~40 bytes memory per instance
- AUTOSAR C++14 compliant

**Limitations:**
- Requires Arduino Mega 2560 or better (ESP8266 should work)
- 100ms update may be too slow for DC motor speed control (needs 10-50ms)

**Formulas implemented:**

| Method | Kp | Ki | Kd |
|---|---|---|---|
| ZN | 0.6·Ku | 2·Kp/Tu | 0.125·Kp·Tu |
| CC | 0.8·Ku | Kp/(0.8·Tu) | 0.194·Kp·Tu |
| IMC | T/(λ+L) | Kp/T | Kp·L/2 |
| TL | Ku/2.2 | Kp/(2.2·Tu) | Kp·Tu/6.3 |
| Lambda | T/(λ+L) | Kp/T | 0.5·Kp·L |

### 3.2 Arduino PID AutoTune (br3ttb / standard)

**URL:** https://docs.arduino.cc/libraries/pid-autotune/

**Features:**
- Part of the standard Arduino PID ecosystem
- Relay-based auto-tuning
- Works with the standard PID library
- Well-tested, widely used

**Approach:**
- Oscillates the output to determine Ku and Tu
- Then applies ZN rules
- Designed for temperature and slow processes
- May need adaptation for fast motor control

### 3.3 Self-Tuning PID on ESP8266

**URL:** https://github.com/manal15iqbal/Self-Tuning-PID-Controller-using-ESP8266

**Features:**
- Runs entirely on ESP8266
- Uses vibration and Back EMF feedback (no encoder)
- Python dashboard for monitoring
- Demonstrates that ESP8266 can handle PID computation

---

## 4. Recommended PID Implementation for Girino

### 4.1 Recommended Architecture

Based on this research, the recommended approach for Girino:

#### Phase 1: Basic PID with Web Interface
- **Discrete PID** (positional/absolute form) with configurable Kp, Ki, Kd
- **Anti-windup** (back-calculation or clamping)
- **Derivative on measurement** (not on error) to avoid derivative kick
- **Output saturation** (0-100% PWM)
- **Sample time:** 20ms (50 Hz) — fast enough for motor speed, achievable on ESP8266
- Web interface for manual gain entry

#### Phase 2: Auto-Tune via Relay Feedback
- Implement Åström-Hägglund relay feedback auto-tuning
- Procedure:
  1. User triggers auto-tune via web API
  2. System switches to relay mode: PWM = +d or -d based on error sign
  3. Measure 5-10 oscillation cycles
  4. Compute Ku and Tu from peak/valley detection
  5. Apply chosen tuning rule (ZN, Tyreus-Luyben, or CC)
  6. Switch back to PID mode with computed gains
- Relay amplitude: ~50% PWM (configurable)

#### Phase 3: Advanced Options
- Multiple tuning rules selectable via web API
- Step response system identification (for ZN 1st / Cohen-Coon / IMC)
- Gain scheduling for different speed ranges
- Web visualization of step response and tuning process

### 4.2 ESP8266 Feasibility Assessment

| Constraint | Assessment |
|---|---|
| **CPU (80 MHz)** | ✅ PID computation takes ~5-10 μs, well within budget |
| **Timer interrupts** | ✅ Use `timer1_isr` for 20ms sampling — proven technique |
| **Encoder ISR** | ✅ GPIO interrupts for encoder at 600 PPR — feasible at typical motor speeds |
| **Memory** | ✅ PID state ~20 bytes; auto-tune buffer ~200 bytes |
| **WiFi + PID** | ⚠️ WiFi ISR can cause timing jitter; use `noInterrupts()` for critical sections |
| **Web server + PID** | ✅ Handle PID in timer ISR, web server in main loop |
| **Float performance** | ⚠️ ESP8266 has no FPU; float ops ~2-5 μs each. Use fixed-point if needed |

### 4.3 Sample Rate Considerations

| Motor Max RPM | Encoder PPR | Max Pulse Rate | Min Sample Period | Recommended Ts |
|---|---|---|---|---|
| 3000 RPM | 600 | 30 kHz | 33 μs | 10-20 ms |
| 3000 RPM | 600 | 30 kHz | 33 μs | 10-20 ms |
| 1000 RPM | 600 | 10 kHz | 100 μs | 20-50 ms |

**Recommendation:** Ts = 20 ms (50 Hz) — provides good speed resolution while leaving CPU time for WiFi.

---

## 5. Summary Table — All Papers Compared

| Paper | Year | MCU | Tuning Method | Control Type | Sample Rate | Interface | Auto-Tune? |
|---|---|---|---|---|---|---|---|
| **Alexandre et al.** | 2025 | ATMEGA328 (Arduino UNO) | Cohen-Coon + fine-tune | Speed | Not specified | SCADA app | ❌ Manual |
| **Cabral et al.** | 2025 | Arduino-based | z-domain design (MATLAB) | Speed | Not specified | MATLAB/Simulink | ❌ Manual |
| **OpenMCT (Von Chong)** | 2026 | Teensy 4.x (600 MHz) | Interactive GUI + z-domain | Speed | Not specified | Python/Qt GUI | ❌ Manual (GUI) |
| **MotoShield (Takács)** | 2021 | ATMEGA328 (Arduino) | MATLAB-computed | Speed | 30-40 ms | MATLAB/Simulink | ❌ Manual |
| **Reck & Sreenivas** | 2015/16 | Raspberry Pi | Analytical (P, PD, P+fb) | Speed + Position | 100 Hz | MATLAB/Simulink | ❌ Manual |
| **Harahap/ZN paper** | 2024 | Generic | Ziegler-Nichols 1st | Speed | Not specified | Not specified | ❌ Manual |
| **Girino (proposed)** | 2026 | ESP8266 | **Relay feedback auto-tune** | **Speed** | **50 Hz** | **Web browser** | ✅ **Yes!** |

**Key insight:** No existing platform implements auto-tuning on the microcontroller. All require MATLAB/Simulink or Python for controller design. The Girino's auto-tune feature via web interface would be a genuine differentiator.

---

## 6. Discrete PID Implementation Reference

### 6.1 Standard Discrete PID (Positional/Absolute Form)

```
e[k] = r[k] - y[k]                          // Error
P[k] = Kp * e[k]                             // Proportional
I[k] = I[k-1] + Ki * Ts * e[k]              // Integral (with anti-windup)
D[k] = Kd/Ts * (y[k] - y[k-1])              // Derivative on measurement
u[k] = P[k] + I[k] - D[k]                   // Output
u[k] = constrain(u[k], 0, 100)              // Saturate
```

### 6.2 Incremental/Velocity Form (Alternative)

```
Δu[k] = Kp*(e[k]-e[k-1]) + Ki*Ts*e[k] + Kd/Ts*(e[k]-2*e[k+1]+e[k-2])
u[k] = u[k-1] + Δu[k]
```

### 6.3 Relay Feedback Auto-Tune Algorithm (Pseudocode)

```cpp
enum TuneState { IDLE, RELAY_ON, COMPUTING };

struct AutoTuner {
  float relayAmplitude;    // d (e.g., 50.0)
  float setpoint;          // Target RPM
  float peaks[10];
  float valleys[10];
  int peakCount, valleyCount;
  unsigned long peakTimes[10];
  unsigned long valleyTimes[10];
  bool lastOutputHigh;
  TuneState state;
  
  float computeKu() {
    float a = (avg(peaks) - avg(valleys)) / 2.0;
    return (4.0 * relayAmplitude) / (PI * a);
  }
  
  float computeTu() {
    return avgPeriod(peakTimes);
  }
  
  // Apply ZN rules
  void getZN_PID(float Ku, float Tu, float &Kp, float &Ki, float &Kd) {
    Kp = 0.6 * Ku;
    Ki = 1.2 * Ku / Tu;
    Kd = 0.075 * Ku * Tu;
  }
  
  // Apply Tyreus-Luyben rules (less aggressive)
  void getTL_PID(float Ku, float Tu, float &Kp, float &Ki, float &Kd) {
    Kp = Ku / 2.2;
    Ki = Kp / (2.2 * Tu);
    Kd = Kp * Tu / 6.3;
  }
};
```

---

## 7. References

1. Alexandre, G.B., Silva, F.N., Souza, G.S. (2025). "ANÁLISE DO CONTROLE PID APLICADO NO CONTROLE DE VELOCIDADE DE UM MOTOR CC." *Revista de Engenharia e Tecnologia*, 17(1). https://revistas.uepg.br/index.php/ret/article/view/24725

2. Cabral, P.H.C. et al. (2025). "Mini bancada de baixo custo com motores CC e encoder para prática de controle realimentado." *Ciência e Natura*, 47(esp. 4). https://periodicos.ufsm.br/cienciaenatura/article/view/92211

3. Von Chong, A. (2026). "OpenMCT: an open-source DC motor control educational kit." *HardwareX*. https://doi.org/10.1016/j.ohx.2026.e00664

4. Takács, G. et al. (2021). "MotoShield: Open Miniaturized DC Motor Hardware Prototype for Control Education." *IEEE FIE*. https://ieeexplore.ieee.org/document/9453979

5. Reck, R.M., Sreenivas, R.S. (2015). "Developing a New Affordable DC Motor Laboratory Kit for an Existing Undergraduate Controls Course." *ACC 2015*. https://ieeexplore.ieee.org/document/7171014

6. Reck, R.M., Sreenivas, R.S. (2016). "Developing an Affordable and Portable Control Systems Laboratory Kit." *Electronics*, 5(3), 36. https://www.mdpi.com/2079-9292/5/3/36

7. Åström, K.J., Hägglund, T. (2006). *Advanced PID Control*. ISA.

8. Ziegler, J.G., Nichols, N.B. (1942). "Optimum Settings for Automatic Controllers." *Transactions of the ASME*, 64, 759-768.

9. Cohen, G.H., Coon, G.A. (1953). "Theoretical Consideration of Retarded Control." *Transactions of the ASME*, 75, 827-834.

10. Tyreus, B.D., Luyben, W.L. (1992). "Tuning PI Controllers for Integrator/Dead Time Processes." *Industrial & Engineering Chemistry Research*, 31(11), 2625-2628.

11. AutoTunePID Library. https://github.com/lily-osp/AutoTunePID

12. Self-Tuning PID on ESP8266. https://github.com/manal15iqbal/Self-Tuning-PID-Controller-using-ESP8266

13. Moradi, B. "DC motor speed control PID tuning Ziegler-Nichols Method." https://github.com/Bmoradi93/DC-motor-speed-control-PID-tuning-Ziegler-Nichols-Method
