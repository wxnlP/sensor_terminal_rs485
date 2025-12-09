#ifndef MAIN_TASKS_H
#define MAIN_TASKS_H



#ifdef __cplusplus
extern "C" {
#endif

void NewWifiStaTask(void);
void NewWifiApTask(void);
#ifdef WX_STATION
void NewWxScreenTask(void);
#endif

#ifdef __c_plusplus
}
#endif

#endif