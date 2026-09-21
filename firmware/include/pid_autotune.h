/**
 * pid_autotune.h — Relay feedback auto-tune (Åström-Hägglund, 1984)
 *
 * How it works (educational):
 *
 * 1. A relay is applied instead of the PID: output alternates between
 *    (bias + d) while RPM < setpoint and (bias - d) while RPM >= setpoint.
 *    The BIAS (pedestal) keeps the motor above its dead zone: a DC motor
 *    needs a minimum PWM to overcome static friction — without the bias,
 *    a relay of d=20% alternated with 0% would just buzz without turning.
 * 2. The dead time of the motor makes the speed oscillate around the
 *    setpoint in a "limit cycle".
 * 3. From the oscillation we identify the process at the stability
 *    limit:
 *        Tu = average period of the oscillation (critical period)
 *        Ku = 4*d / (pi*a)          (critical gain)
 *    where d is the actual relay step (uHigh - uLow)/2 in % PWM and
 *    a is the oscillation amplitude (RPM).
 * 4. Classic rules (ZN, TL, CC) are then applied to Ku and Tu.
 */

#ifndef PID_AUTOTUNE_H
#define PID_AUTOTUNE_H

#include "config.h"

// State machine of the experiment
enum AutotuneState {
    AUTOTUNE_IDLE,     // not running
    AUTOTUNE_RUNNING,  // relay applied, collecting oscillation data
    AUTOTUNE_DONE,     // finished, results valid
    AUTOTUNE_FAILED,   // timeout or no oscillation
};

/**
 * Initializes the module (idle state).
 */
void autotuneInit();

/**
 * Starts a relay feedback experiment.
 * @param relayAmp Relay amplitude d in % PWM (5-100)
 * @param bias Pedestal in % PWM (0-60). Use ~the dead zone of the motor.
 * @param cycles Number of full periods used for averaging (3-5)
 * @param setpoint Operating point in RPM around which the system oscillates
 */
void autotuneStart(float relayAmp, float bias, int cycles, float setpoint);

/**
 * Cancels a running experiment (safety stop).
 */
void autotuneCancel();

/**
 * One step of the experiment. Must be called at a fixed rate
 * (PID_SAMPLE_MS) with the current speed measurement.
 *
 * @param measurement Current speed in RPM
 * @param dt Sample period in seconds
 */
void autotuneCompute(float measurement, float dt);

/**
 * @return Current relay output in % (0 or d) — apply this to the motor.
 */
float autotuneGetRelayOutput();

/**
 * @return true while the experiment is running.
 */
bool autotuneIsRunning();

/**
 * @return true when the experiment finished successfully.
 */
bool autotuneIsDone();

/**
 * @return true if the experiment failed (timeout).
 */
bool autotuneIsFailed();

/**
 * @return Identification results. Valid only after autotuneIsDone().
 *         Set 'valid' by autotuneIsDone() — ku/tu are the last results.
 */
float autotuneGetKu();
float autotuneGetTu();

/**
 * @return Progress 0-100 (%) of the experiment.
 */
int autotuneGetProgress();

#endif // PID_AUTOTUNE_H
