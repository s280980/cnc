#ifndef protocol_h


//pc-->nc
#define CMD_LINK 128
#define CMD_WRITE_PARAMS_TO_EEPROM 133
#define CMD_RESET 134
#define CMD_TASK_RUNNING_STATE_REP_DT_SET 135
#define CMD_STEPPER_POSITION_REP_DT_SET 136
#define CMD_TASK 137
#define CMD_MODE 138
#define CMD_STEPPERS 139
#define CMD_AXIS 140
#define CMD_JOG_RATE_MAX 141


//nc-->pc
//+#define CMD_LINK 128
#define CMD_TASK_RUNNING_STATE 135
#define CMD_STEPPER_POSITION 136
#define CMD_TASK_ACCEPTED 137
#define CMD_MODE_STATE 138
#define CMD_STEPPERS_STATE 139
//+#define CMD_AXIS 140


void protocol_process_input();


#define protocol_h
#endif
