/**
 * pid_tuning_rules.h — Classic tuning rules from relay feedback data
 *
 * Given the critical gain (Ku) and critical period (Tu) obtained by the
 * relay feedback experiment (Åström-Hägglund), computes PID gains for:
 *
 *   ZN — Ziegler-Nichols  (fast, oscillatory)
 *   TL — Tyreus-Luyben    (slower, less overshoot)
 *   CC — Cohen-Coon       (intermediate)
 */

#ifndef PID_TUNING_RULES_H
#define PID_TUNING_RULES_H

struct PidGains {
    float kp;
    float ki;
    float kd;
};

/**
 * Ziegler-Nichols: Kp = 0.6*Ku, Ti = Tu/2, Td = Tu/8
 */
PidGains tuningZN(float ku, float tu);

/**
 * Tyreus-Luyben: Kp = Ku/2.2, Ti = 2.2*Tu, Td = Tu/6.3
 */
PidGains tuningTL(float ku, float tu);

/**
 * Cohen-Coon (open-loop rule adapted for relay data):
 * Kp = (Ku/1.35), Ti = 0.5*Tu, Td = 0.25*Tu
 */
PidGains tuningCC(float ku, float tu);

#endif // PID_TUNING_RULES_H
