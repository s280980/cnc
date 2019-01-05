#ifndef stepper_h
#include <stdint.h>
#include "config.h"

#define ST_MODE_STOPPED 0
#define ST_MODE_ACC 1
#define ST_MODE_CRUISE 2
#define ST_MODE_DEC 3


struct frep_time_t{
  uint32_t t;
  uint32_t dt;
  };


struct stepper_params_t{
  uint8_t  stepbits_invert_mask;
  uint16_t rate_min;
  uint32_t st_inv_acc;
  uint16_t tmr_dt[TMR_COUNT];
  };



struct task_t{
  uint8_t id;
  uint8_t dirbits;
  int16_t err[NAXIS];
  int16_t step[NAXIS]; // axis
  int16_t time; //time
  int16_t steps; //step_count
  int16_t steps_dec;
  uint16_t rate;
  };


void stepper_init();
void stepper_stop();

task_t* task_reserve_cell();
void task_apply(task_t* t);
void tasks_execute();
void report_task_running_state(uint32_t ms_time);
void report_stepper_position(uint32_t ms_time);

extern stepper_params_t params;

#define stepper_h
#endif