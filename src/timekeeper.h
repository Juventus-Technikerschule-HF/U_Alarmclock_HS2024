#pragma once

#include "freertos/FreeRTOS.h"

typedef struct {
    struct {
        uint8_t hour;
        uint8_t min;
        uint8_t sec;
    }time;
    struct {
        uint16_t year;
        uint8_t month;
        uint8_t day;
    }date;
}systime_t;


void init_timekeeper(void);
void timekeeper_SetTime(systime_t *time);
void timekeeper_GetTime(systime_t *returntime);
bool timekeeper_TimeInitialized(void);
int timekeeper_GetUTCDeviation();