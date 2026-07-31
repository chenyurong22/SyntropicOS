#ifndef MOCK_FREERTOS_EVENT_GROUPS_H
#define MOCK_FREERTOS_EVENT_GROUPS_H

#include "FreeRTOS.h"

typedef void *EventGroupHandle_t;
typedef uint32_t EventBits_t;

static inline EventGroupHandle_t xEventGroupCreate(void)
{
    return (EventGroupHandle_t)1;
}
static inline EventBits_t xEventGroupSetBits(EventGroupHandle_t e, EventBits_t b)
{
    (void)e;
    return b;
}
static inline EventBits_t xEventGroupClearBits(EventGroupHandle_t e, EventBits_t b)
{
    (void)e;
    return b;
}
static inline EventBits_t xEventGroupWaitBits(EventGroupHandle_t e, EventBits_t b, bool c, bool w,
                                              TickType_t t)
{
    (void)e;
    (void)b;
    (void)c;
    (void)w;
    (void)t;
    return 0;
}

#endif /* MOCK_FREERTOS_EVENT_GROUPS_H */
