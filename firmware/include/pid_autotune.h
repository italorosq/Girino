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
 * Which closed loop is being identified:
 *
 *   SPEED    — unidirectional relay (bias ± d) around an RPM setpoint.
 *              The bias keeps the motor above its dead zone.
 *   POSITION — symmetric relay (+d forward / -d reverse) around an
 *              angle target (degrees). The plant is bidirectional, so
 *              no bias is needed; d must exceed the motor dead zone
 *              (~60% on this bench) for both relay states to move.
 */
enum AutotunePlant {
    AUTOTUNE_PLANT_SPEED,
    AUTOTUNE_PLANT_POSITION,
};

/**
 * Initializes the module (idle state).
 */
void autotuneInit();

/**
 * Starts a relay feedback experiment.
 * @param plant Which loop is identified (speed or position)
 * @param relayAmp Relay amplitude d in % PWM (5-100)
 * @param bias Pedestal in % PWM (0-95). Only used for SPEED; ignored
 *             for POSITION (symmetric relay around zero).
 * @param cycles Number of full periods used for averaging (3-5)
 * @param setpoint Operating point: RPM (speed) or degrees (position)
 */
void autotuneStart(AutotunePlant plant, float relayAmp, float bias, int cycles, float setpoint);

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
 * @return Current relay output in % — SPEED: 0..100 (forward only);
 *         POSITION: -d..+d with sign (negative = reverse).
 *         Apply the magnitude to the motor and the sign to the direction.
 */
float autotuneGetRelayOutput();

/**
 * @return Which plant the experiment identifies.
 */
AutotunePlant autotuneGetPlant();

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
