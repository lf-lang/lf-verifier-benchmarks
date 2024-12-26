#ifndef SAT_ATTITUDE_CONTROLLER_H
#define SAT_ATTITUDE_CONTROLLER_H

#include <stdint.h>

typedef struct {
  int x;
  int y;
  int z;
} IntVec3;

// Fixed point confuguration
#define FIXED_POINT_FRACTION_BITS 12
#define FIXED_POINT_SCALE (1 << FIXED_POINT_FRACTION_BITS)

void gyro_reaction(IntVec3 *sample_ret);

typedef struct {
  IntVec3 last_angle;
  unsigned int delta_t;
} SensorFusionState;

void sensor_fusion_startup_reaction(SensorFusionState *state, int64_t delta_t);

void sensor_fusion_reaction(SensorFusionState *state,
                            IntVec3 *gyro1,
                            IntVec3 *gyro2, IntVec3 *angle,
                            IntVec3 *angular_speed);

typedef struct {
  IntVec3 last_error;
  IntVec3 error_accumulated;
  IntVec3 desired_angle;
  int Kp;
  int Ki;
  int Kd;
} ControllerState;

void controller_run_reaction(ControllerState *state, IntVec3 *current_angle,
                             IntVec3 *current_angular_speed,
                             IntVec3 *motor_ret);
void controller_startup_reaction(ControllerState *state, float Kp, float Ki,
                                 float Kd);

void controller_user_input_reaction(ControllerState *state,
                                    IntVec3 *user_input);

void motor_reaction(IntVec3 *control_signal);

void user_input_startup(IntVec3 *desired_angle);

#endif