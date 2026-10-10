#pragma once

enum { Q2_CONTROL_MENU = 1, Q2_CONTROL_OPTIONS, Q2_CONTROL_EXIT, Q2_CONTROL_RESUME_NOTICE, Q2_CONTROL_ERROR, Q2_CONTROL_SAVE_NOTICE };
void q2_control_init(void (*handler)(int, const char *));
void q2_control_request(int action, const char *message);
_Noreturn void q2_control_run(void);
