#include <avr/io.h>
#include <avr/sleep.h>
#include "serial.h"
#include "protocol.h"
#include "stepper.h"
#include "io.h"


void setup(){
  serial_init();
  stepper_init();
  io_init();
  }//setup



void loop(){
  //sleep_mode();
  protocol_process_input();
  stepper_loop();
  //report_task_running_state(millis());
  report_stepper_position(millis());
  }//loop
