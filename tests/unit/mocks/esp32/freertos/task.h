#ifndef MOCK_FREERTOS_TASK_H
#define MOCK_FREERTOS_TASK_H

#include "FreeRTOS.h"

static inline void vTaskDelete(TaskHandle_t t)
{
    (void)t;
}

#endif /* MOCK_FREERTOS_TASK_H */
