#ifndef _TASK_MASTER_H_
#define _TASK_MASTER_H_

#define TaskStatusStop 0
#define TaskStatusRunning 1
#define TaskStatusSleep 2

#define TaskTypeRepeating 0
#define TaskTypeTimeout 1

#define MaxTaskCount	50

typedef struct Task{
	char title[50];
	int32_t status; //0:stop 1:running 2:sleep
	int32_t type; //repeat, timeout
	int32_t periodMs; //execute every x ms
	int32_t tLastExecutionMs;
} Task;


 /*BaseType_t xTaskCreate(
							  TaskFunction_t pvTaskCode,
							  const char * const pcName,
							  configSTACK_DEPTH_TYPE usStackDepth,
							  void *pvParameters,
							  UBaseType_t uxPriority,
							  TaskHandle_t *pvCreatedTask
						  );*/

extern int tmCreateTask();

#endif //_TASK_MASTER_H_