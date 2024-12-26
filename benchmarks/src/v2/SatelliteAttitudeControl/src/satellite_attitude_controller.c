#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

#include "satellite_attitude_controller.h"
#include "synthetic_data.h"

// Convenience macro to simulate some execution time.
#define TAKE_TIME(N)              \
  do {                            \
    volatile int __x;             \
    for (int i = 0; i < N; i++) { \
      __x++;                      \
    }                             \
  } while (0)

// Integer multiplication using bitwise operations. Needed because platin can
// not handle the libc implementations.
unsigned int multiply(unsigned int a, unsigned int b) {
  // printf("a: %d, b: %d\n", a, b);
  int result = 0;

#pragma loopbound min 1 max 32
  while (b > 0) {
    if (b & 1) {
      result += a;
      // printf("b=1 result=%d\n",result);
    } else {
      // printf("b=0\n");
    }
    a <<= 1;
    b >>= 1;
  }
  // printf("result: %d\n", result);

  return result;
}

unsigned _divide(unsigned dividend, unsigned divisor) {
  unsigned current = 1;
  unsigned answer = 0;
  unsigned ret = 0;


  if (divisor == dividend) {
    return 1;
  }

#pragma loopbound min 1 max 32
  while (divisor <= dividend) {
    divisor <<= 1;
    current <<= 1;
  }

  divisor >>= 1;
  current >>= 1;

#pragma loopbound min 1 max 32
  while (current != 0) {
    if (dividend >= divisor) {
      dividend -= divisor;
      answer |= current;
    }
    current >>= 1;
    divisor >>= 1;
  }
  return answer;
}

// Integer division using bitwise operations. Needed because platin can not
// handle the libc implementations.
unsigned divide(unsigned dividend, unsigned divisor) {
  if ( divisor > dividend)
  {
      return 0;
  } else {
      return _divide(dividend, divisor);
  }
}

void gyro_reaction(IntVec3 *sample_ret) {
  static int i = 0;

  int base_idx = multiply(i, 3);
  int *sample = synthetic_gyro[base_idx];
  sample_ret->x = *sample;
  sample++;
  sample_ret->y = *sample;
  sample++;
  sample_ret->z = *sample;
  if (i++ == NUM_GYRO_SAMPLES) {
    i = 0;
  }
}

void sensor_fusion_reaction(SensorFusionState *state,
                            IntVec3 *gyro1,
                            IntVec3 *gyro2, IntVec3 *angle,
                            IntVec3 *angular_speed) {
  IntVec3 gyro_avg = {(gyro1->x + gyro2->x) >> 1, (gyro1->y + gyro2->y) >> 1,
                      (gyro1->z + gyro2->z) >> 1};

  angle->x = state->last_angle.x +
             (multiply(gyro_avg.x, state->delta_t) >> FIXED_POINT_FRACTION_BITS);
  angle->y = state->last_angle.y +
             (multiply(gyro_avg.y, state->delta_t) >> FIXED_POINT_FRACTION_BITS);
  angle->z = state->last_angle.z +
             (multiply(gyro_avg.z, state->delta_t) >> FIXED_POINT_FRACTION_BITS);

  angular_speed->x = gyro_avg.x;
  angular_speed->y = gyro_avg.y;
  angular_speed->z = gyro_avg.z;

  state->last_angle.x = angle->x;
  state->last_angle.y = angle->y;
  state->last_angle.z = angle->z;
}

void controller_run_reaction(ControllerState *state, IntVec3 *current_angle,
                             IntVec3 *current_angular_speed,
                             IntVec3 *motor_ret) {
  (void)current_angular_speed;
  IntVec3 error;

  error.x = (current_angle->x - state->desired_angle.x);
  error.y = (current_angle->y - state->desired_angle.y);
  error.z = (current_angle->z - state->desired_angle.z);

  state->error_accumulated.x += error.x;
  state->error_accumulated.y += error.y;
  state->error_accumulated.z += error.z;

  motor_ret->x = (multiply(state->Kp, error.x) >> FIXED_POINT_FRACTION_BITS) +
                 (multiply(state->Kp, state->error_accumulated.x) >>
                  FIXED_POINT_FRACTION_BITS)  // integral component
                 + (multiply(state->Kd, (state->last_error.x - error.x)) >>
                    FIXED_POINT_FRACTION_BITS);  // differential component
  motor_ret->y = (multiply(state->Kp, error.y) >> FIXED_POINT_FRACTION_BITS) +
                 (multiply(state->Ki, state->error_accumulated.y) >>
                  FIXED_POINT_FRACTION_BITS)  // integral component
                 + (multiply(state->Kd, (state->last_error.y - error.y)) >>
                    FIXED_POINT_FRACTION_BITS);  // differential component
  motor_ret->z = (multiply(state->Kp, error.z) >> FIXED_POINT_FRACTION_BITS) +
                 (multiply(state->Ki, state->error_accumulated.z) >>
                  FIXED_POINT_FRACTION_BITS)  // integral component
                 + (multiply(state->Kd, (state->last_error.z - error.z)) >>
                    FIXED_POINT_FRACTION_BITS);

  state->last_error.x = error.x;
  state->last_error.y = error.y;
  state->last_error.z = error.z;
}
void controller_startup_reaction(ControllerState *state, float Kp, float Ki,
                                 float Kd) {
  state->Kp = Kp * FIXED_POINT_SCALE;
  state->Ki = Ki * FIXED_POINT_SCALE;
  state->Kd = Kd * FIXED_POINT_SCALE;
}

void controller_user_input_reaction(ControllerState *state,
                                    IntVec3 *user_input) {
  state->desired_angle.x = user_input->x;
  state->desired_angle.y = user_input->y;
  state->desired_angle.z = user_input->z;
}

void motor_reaction(IntVec3 *control_signal) {
  (void)control_signal;
}

void user_input_startup(IntVec3 *desired_angle) {
  desired_angle->x = 4096;
  desired_angle->y = 2048;
  desired_angle->z = 819;
}