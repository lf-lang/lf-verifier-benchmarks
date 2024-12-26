#ifndef INSTRUMENTATION_H
#define INSTRUMENTATION_H

// Conveneince macros for calculating the current lag
#define LAG() (rdtime() - ((uint32_t)lf_time_logical()))

// Convenience macro for printing out the lag of a module. Uncomment to enable
// #define PRINT_LAGS
#ifdef PRINT_LAGS
#define PRINT_LAG(module) lf_print("Lag in " #module " = %lu", LAG())
#else
#define PRINT_LAG(module)
#endif

// Convenience macro to measure execution times of a module. Uncomment to enable
#ifdef MEASURE_EXECUTION_TIMES
#define EXEC_BEGIN() uint32_t __t1 = rdtime()
#define EXEC_END()                                                             \
  do {                                                                         \
    uint32_t __t2 = rdtime();                                                  \
    if (self->num_iter >= NUM_ITER) return;                                    \
    self->execution_time[self->num_iter] = __t2 - __t1;                        \
    self->completion_lag[self->num_iter] = __t2 - (uint32_t)lf_time_logical(); \
    self->start_lag[self->num_iter] = __t1 - (uint32_t)lf_time_logical();      \
    self->num_iter++;                                                          \
  } while (0)

#define EXEC_REPORT(module)                                                   \
  do {                                                                        \
    lf_print("Module=" #module);                                              \
    for (int i = 0; i < NUM_ITER; i++) {                                      \
      lf_print(                                                               \
          "Iteration_%i Start_lag=%d Execution_time=%d Completion_lag=%d", i, \
          self->start_lag[i], self->execution_time[i],                        \
          self->completion_lag[i]);                                           \
    }                                                                         \
  } while (0)
#else

#define EXEC_BEGIN()
#define EXEC_END()
#define EXEC_REPORT(module)
#endif

#endif
