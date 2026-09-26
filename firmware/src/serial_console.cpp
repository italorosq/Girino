/**
 * serial_console.cpp — Console de comandos via serial (USB)
 *
 * Comandos disponíveis (digite 'help'):
 *
 *   status                          modo, RPM, ângulo, pulsos, heap, IP
 *   enc                             leitura do encoder agora
 *   motor <fwd|rev|stop> [0-100]    comando manual (cancela controladores)
 *   pid <kp> <ki> <kd> <sp>         configura PID de velocidade (RPM)
 *   pid start | pid stop            liga/desliga a malha de velocidade
 *   pos <kp> <ki> <kd> <graus>      configura PID de posição
 *   pos start | pos stop | pos zero
 *   autotune <amp> <bias> <n> <sp>  inicia relay feedback (velocidade)
 *   autotune pos <amp> <n> <graus>  inicia relay feedback (posição)
 *   autotune cancel                 cancela o experimento
 *   tune                            Ku/Tu e ganhos ZN/TL/CC (após autotune)
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "serial_console.h"
#include "config.h"
#include "motor_control.h"
#include "encoder.h"
#include "pid.h"
#include "pid_autotune.h"
#include "pid_tuning_rules.h"

// Definidos em main.cpp
extern void resetResponseBuffer();
extern void setResponseUnit(const char* unit);

static void printHelp() {
    Serial.println();
    Serial.println("=== Comandos do Girino ===");
    Serial.println("  help                             esta ajuda");
    Serial.println("  status                           modo, RPM, angulo, pulsos, heap, IP");
    Serial.println("  enc                              leitura do encoder agora");
    Serial.println("  motor <fwd|rev|stop> [0-100]     comando manual (cancela controladores)");
    Serial.println("  pid <kp> <ki> <kd> <sp>          configura PID de velocidade (RPM)");
    Serial.println("  pid start | pid stop             liga/desliga a malha de velocidade");
    Serial.println("  pos <kp> <ki> <kd> <graus>       configura PID de posicao");
    Serial.println("  pos start | pos stop | pos zero  controle de posicao / zera origem");
 Serial.println("  autotune <amp> <bias> <n> <sp>   inicia relay feedback (velocidade)");
 Serial.println("  autotune pos <amp> <n> <graus>   inicia relay feedback (posição)");
 Serial.println("  autotune cancel                  cancela o experimento");
    Serial.println("  tune                             Ku/Tu e ganhos ZN/TL/CC");
    Serial.println();
}

static const char* modeName() {
    if (autotuneIsRunning()) return "autotune";
    if (posIsRunning()) return "position";
    if (pidIsRunning()) return "pid";
    return "open-loop";
}

static void printStatus() {
    Serial.printf("[Status] modo=%s rpm=%.1f angulo=%.1f pulsos=%ld\n",
                  modeName(), encoderGetRPM(), encoderGetAngle(), encoderGetPulses());
    Serial.printf("[Status] heap=%u IP=%s clientes=%d\n",
                  ESP.getFreeHeap(), WiFi.softAPIP().toString().c_str(),
                  WiFi.softAPgetStationNum());
    Serial.printf("[Status] PWM=%d Hz range=%d | motor: dir=%d speed=%d%%\n",
                  MOTOR_PWM_FREQ, MOTOR_PWM_RANGE,
                  motorGetDirection(), motorGetSpeed());
    Serial.printf("[Status] fiacao: sentido %s\n",
                  motorDirInverted() ? "INVERTIDO (corrigido em software)" : "normal");
}

static void handleCommand(char* line) {
    char* cmd = strtok(line, " \t");
    if (!cmd) return;

    if (!strcmp(cmd, "help")) {
        printHelp();
        return;
    }

    if (!strcmp(cmd, "status")) {
        printStatus();
        return;
    }

    if (!strcmp(cmd, "enc")) {
        Serial.printf("[Enc] pulsos=%ld rpm=%.1f angulo=%.1f\n",
                      encoderGetPulses(), encoderGetRPM(), encoderGetAngle());
        return;
    }

    // --- motor <fwd|rev|stop> [0-100] ---
    if (!strcmp(cmd, "motor")) {
        char* dir = strtok(NULL, " \t");
        char* spd = strtok(NULL, " \t");
        if (!dir) {
            Serial.println("[Motor] uso: motor <fwd|rev|stop> [0-100]");
            return;
        }
        pidStop();
        autotuneCancel();
        posStop();

        int speed = spd ? atoi(spd) : 0;
        if (!strcmp(dir, "fwd")) {
            motorSetDirection(MOTOR_DIR_FORWARD);
            motorSetSpeed(speed);
        } else if (!strcmp(dir, "rev")) {
            motorSetDirection(MOTOR_DIR_REVERSE);
            motorSetSpeed(speed);
        } else {
            motorSetSpeed(0);
            motorSetDirection(MOTOR_DIR_STOP);
            dir = (char*)"stop";
        }
        Serial.printf("[Motor] dir=%s speed=%d%%\n", dir, speed);
        return;
    }

    // --- pid <kp> <ki> <kd> <sp> | pid start | pid stop ---
    if (!strcmp(cmd, "pid")) {
        char* a1 = strtok(NULL, " \t");
        if (a1 && !strcmp(a1, "start")) {
            posStop();
            autotuneCancel();
            setResponseUnit("rpm");
            resetResponseBuffer();
            pidStart();
            Serial.println("[PID] start");
            return;
        }
        if (a1 && !strcmp(a1, "stop")) {
            pidStop();
            motorSetSpeed(0);
            motorSetDirection(MOTOR_DIR_STOP);
            Serial.println("[PID] stop");
            return;
        }
        char* a2 = strtok(NULL, " \t");
        char* a3 = strtok(NULL, " \t");
        char* a4 = strtok(NULL, " \t");
        if (!a1 || !a2 || !a3 || !a4) {
            Serial.println("[PID] uso: pid <kp> <ki> <kd> <sp> | pid start | pid stop");
            return;
        }
        pidConfigure(atof(a1), atof(a2), atof(a3), atof(a4));
        return;
    }

    // --- pos ... ---
    if (!strcmp(cmd, "pos")) {
        char* a1 = strtok(NULL, " \t");
        if (a1 && !strcmp(a1, "start")) {
            pidStop();
            autotuneCancel();
            setResponseUnit("deg");
            resetResponseBuffer();
            posStart();
            Serial.println("[Pos] start");
            return;
        }
        if (a1 && !strcmp(a1, "stop")) {
            posStop();
            motorSetSpeed(0);
            motorSetDirection(MOTOR_DIR_STOP);
            Serial.println("[Pos] stop");
            return;
        }
        if (a1 && !strcmp(a1, "zero")) {
            posStop();
            motorSetSpeed(0);
            motorSetDirection(MOTOR_DIR_STOP);
            encoderReset();
            Serial.println("[Pos] zero (0 graus)");
            return;
        }
        char* a2 = strtok(NULL, " \t");
        char* a3 = strtok(NULL, " \t");
        char* a4 = strtok(NULL, " \t");
        if (!a1 || !a2 || !a3 || !a4) {
            Serial.println("[Pos] uso: pos <kp> <ki> <kd> <graus> | pos start | pos stop | pos zero");
            return;
        }
        posConfigure(atof(a1), atof(a2), atof(a3), atof(a4));
        return;
    }

    // --- autotune [pos] <amp> ... | autotune cancel ---
    if (!strcmp(cmd, "autotune")) {
        char* a1 = strtok(NULL, " \t");
        if (a1 && !strcmp(a1, "cancel")) {
            autotuneCancel();
            motorSetSpeed(0);
            motorSetDirection(MOTOR_DIR_STOP);
            Serial.println("[AutoTune] cancelado");
            return;
        }

        // autotune pos <amp> <n> <graus> — relay simétrico ±d na posição
        if (a1 && !strcmp(a1, "pos")) {
            char* b1 = strtok(NULL, " \t");
            float amp = b1 ? atof(b1) : (float)AUTOTUNE_POS_DEFAULT_RELAY;
            char* b2 = strtok(NULL, " \t");
            int cycles = b2 ? atoi(b2) : AUTOTUNE_DEFAULT_CYCLES;
            char* b3 = strtok(NULL, " \t");
            float target = b3 ? atof(b3) : posGetTarget();

            if (amp < AUTOTUNE_POS_MIN_RELAY || amp > 100.0 ||
                cycles < 1 || cycles > 8 ||
                target < -POS_MAX_TARGET_DEG || target > POS_MAX_TARGET_DEG) {
                Serial.println("[AutoTune] parametros invalidos");
                return;
            }
            pidStop();
            posStop();
            setResponseUnit("deg");
            resetResponseBuffer();
            autotuneStart(AUTOTUNE_PLANT_POSITION, amp, 0.0, cycles, target);
            return;
        }

        // autotune <amp> <bias> <n> <sp> — relay com bias na velocidade
        float amp = a1 ? atof(a1) : (float)AUTOTUNE_DEFAULT_RELAY;
        char* a2 = strtok(NULL, " \t");
        float bias = a2 ? atof(a2) : (float)AUTOTUNE_DEFAULT_BIAS;
        char* a3 = strtok(NULL, " \t");
        int cycles = a3 ? atoi(a3) : AUTOTUNE_DEFAULT_CYCLES;
        char* a4 = strtok(NULL, " \t");
        float sp = a4 ? atof(a4) : AUTOTUNE_DEFAULT_SETPOINT;

        if (amp < 1.0 || bias < 0.0 || bias > AUTOTUNE_BIAS_MAX ||
            cycles < 1 || cycles > 8 || sp <= 0.0) {
            Serial.println("[AutoTune] parametros invalidos");
            return;
        }
        pidStop();
        posStop();
        setResponseUnit("rpm");
        resetResponseBuffer();
        autotuneStart(AUTOTUNE_PLANT_SPEED, amp, bias, cycles, sp);
        return;
    }

    // --- tune ---
    if (!strcmp(cmd, "tune")) {
        if (!autotuneIsDone()) {
            Serial.println("[Tune] rode 'autotune' primeiro");
            return;
        }
        float ku = autotuneGetKu();
        float tu = autotuneGetTu();
        PidGains zn = tuningZN(ku, tu);
        PidGains tl = tuningTL(ku, tu);
        PidGains cc = tuningCC(ku, tu);
        Serial.printf("[Tune] planta=%s Ku=%.3f Tu=%.3f s\n",
                      autotuneGetPlant() == AUTOTUNE_PLANT_POSITION ? "posicao" : "velocidade",
                      ku, tu);
        Serial.printf("[Tune] ZN Kp=%.3f Ki=%.3f Kd=%.3f\n", zn.kp, zn.ki, zn.kd);
        Serial.printf("[Tune] TL Kp=%.3f Ki=%.3f Kd=%.3f\n", tl.kp, tl.ki, tl.kd);
        Serial.printf("[Tune] CC Kp=%.3f Ki=%.3f Kd=%.3f\n", cc.kp, cc.ki, cc.kd);
        return;
    }

    Serial.printf("Comando desconhecido: %s (digite 'help')\n", cmd);
}

void serialConsoleInit() {
    Serial.println("[Console] Digite 'help' para os comandos (115200 baud)");
    Serial.print("> ");
}

void serialConsoleUpdate() {
    static char buf[128];
    static uint8_t len = 0;

    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            buf[len] = '\0';
            if (len > 0) {
                handleCommand(buf);
                len = 0;
            }
            Serial.print("> ");
        } else if (len < sizeof(buf) - 1) {
            buf[len++] = c;
        }
    }
}
