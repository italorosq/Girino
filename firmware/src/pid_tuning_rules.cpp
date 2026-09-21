/**
 * pid_tuning_rules.cpp — Classic tuning rules implementation
 *
 * All rules derive Kp, Ki, Kd from the relay feedback experiment:
 *
 *   Kp = Kp rule
 *   Ki = Kp / Ti   (integral time Ti in seconds)
 *   Kd = Kp * Td   (derivative time Td in seconds)
 *
 * Educational note: each rule is a different trade-off between speed
 * and robustness. ZN gives the fastest response but overshoots; TL
 * trades settling time for smoothness; CC sits in between.
 */

#include "pid_tuning_rules.h"

PidGains tuningZN(float ku, float tu) {
    float kp = 0.6 * ku;
    float ti = tu / 2.0; // Ti = Tu/2
    float td = tu / 8.0; // Td = Tu/8
    PidGains g;
    g.kp = kp;
    g.ki = kp / ti;
    g.kd = kp * td;
    return g;
}

PidGains tuningTL(float ku, float tu) {
    float kp = ku / 2.2;
    float ti = 2.2 * tu;  // Ti = 2.2*Tu
    float td = tu / 6.3;  // Td = Tu/6.3
    PidGains g;
    g.kp = kp;
    g.ki = kp / ti;
    g.kd = kp * td;
    return g;
}

PidGains tuningCC(float ku, float tu) {
    // Simplified CC for relay data (no dead time estimation):
    // same Kp factor as the full rule with dead time ~ Tu/8
    float kp = ku / 1.35;
    float ti = 0.5 * tu;  // Ti = 0.5*Tu
    float td = 0.25 * tu; // Td = 0.25*Tu
    PidGains g;
    g.kp = kp;
    g.ki = kp / ti;
    g.kd = kp * td;
    return g;
}
