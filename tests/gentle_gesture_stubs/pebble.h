#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
typedef struct { int unused; } AppTimer;
typedef struct { int16_t x, y, z; bool did_vibrate; uint64_t timestamp; } AccelData;
enum { ACCEL_SAMPLING_50HZ = 50 };
void app_timer_cancel(AppTimer *timer);
AppTimer *app_timer_register(uint32_t ms, void (*callback)(void *), void *context);
void accel_data_service_subscribe(uint32_t count, void (*callback)(AccelData *, uint32_t));
void accel_data_service_unsubscribe(void);
int accel_service_set_sampling_rate(int rate);
