/********************************************************************************************* */
//    Eduboard2 ESP32-S3 Template with BSP
//    Author: Martin Burger
//    Juventus Technikerschule
//    Version: 1.0.0
//    
//    This is the ideal starting point for a new Project. BSP for most of the Eduboard2
//    Hardware is included under components/eduboard2.
//    Hardware support can be activated/deactivated in components/eduboard2/eduboard2_config.h
/********************************************************************************************* */
#include "eduboard2.h"
#include "eduboard2/eduboardLCD/src/decode_jpeg.h"
#include "memon.h"

#include "math.h"

#define TAG "ALARMCLOCK_HS2024"

#define UPDATETIME_MS 100

#define TIMEMODE_RTC


#define EV_SW0_SHORT            0x01 << 0
#define EV_SW1_SHORT            0x01 << 1
#define EV_SW2_SHORT            0x01 << 2
#define EV_SW3_SHORT            0x01 << 3
#define EV_SW0_LONG             0x01 << 4
#define EV_SW1_LONG             0x01 << 5
#define EV_SW2_LONG             0x01 << 6
#define EV_SW3_LONG             0x01 << 7
#define EV_ALL_SWITCH           0xFF

#define EV_TIMEUPDATE           0x01 << 8
#define EV_SCREENUPDATE_IDLE    0x01 << 9
EventGroupHandle_t ev_alarmclock_status;

typedef enum {
    TIMESELECT_HOUR = 0,
    TIMESELECT_MIN = 1,
    TIMESELECT_SEC = 2,
} timeselection_t;

uint8_t hour = 0;
uint8_t min = 0;
uint8_t sec = 0;

uint8_t alarm_hour = 0;
uint8_t alarm_min  = 0;
uint8_t alarm_sec  = 0;

void alarmclock_timing_task(void* param) {
    for(;;) {
        if(xEventGroupGetBits(ev_alarmclock_status) & EV_TIMEUPDATE) {
            #ifdef TIMEMODE_RTC
            rtc_get_time(&hour, &min, &sec);
            vTaskDelay(500/portTICK_PERIOD_MS);
            #else   
            sec++;
            if(sec >= 60) {
                sec = 0;
                min++;
            }
            if(min >= 60) {
                min = 0;
                hour++;
            }
            if(hour >= 24) {
                hour = 0;
            }
            vTaskDelay(1000/portTICK_PERIOD_MS);
            // vTaskDelay(10/portTICK_PERIOD_MS);
            #endif
        } else {
            vTaskDelay(500/portTICK_PERIOD_MS);
        }
    }
}

void drawAlarmSymbol(uint16_t start_x, uint16_t start_y) {
    pixel_jpeg **pixels;
	uint16_t imageWidth;
	uint16_t imageHeight;
	uint16_t width = 100;
	uint16_t height = 100;
	char file[32];
	strcpy(file, "/spiffs/alarmsymbol.jpg");
	esp_err_t err = decode_jpeg(&pixels, file, width, height, &imageWidth, &imageHeight);
	if (err == ESP_OK) {
		uint16_t _width = width;
		uint16_t _cols = 0;
		if (width > imageWidth) {
			_width = imageWidth;
			_cols = (width - imageWidth) / 2;
		}
		uint16_t _height = height;
		uint16_t _rows = 0;
		if (height > imageHeight) {
			_height = imageHeight;
			_rows = (height - imageHeight) / 2;
		}
        _cols+=start_x;
        _rows+=start_y;
		uint16_t *colors = (uint16_t*)malloc(sizeof(uint16_t) * _width);
		for(int y = 0; y < _height; y++){
			for(int x = 0;x < _width; x++){
				colors[x] = pixels[y][x];
			}
			lcdDrawMultiPixels(_cols, y+_rows, _width, colors);
		}
		free(colors);
		release_image(&pixels, width, height);
	}
}

void setTime(uint8_t _hour, uint8_t _min, uint8_t _sec) {
    #ifdef TIMEMODE_RTC
    rtc_set_time(_hour, _min, _sec);
    #else   
    hour = _hour;
    min  = _min;
    sec  = _sec;
    #endif
}
void setAlarmTime(uint8_t _hour, uint8_t _min, uint8_t _sec) {
    alarm_hour = _hour;
    alarm_min = _min;
    alarm_sec = _sec;
}

void showMainScreen(bool alarmOn) {
    lcdDrawString(fx32L, 30,30,"AlarmClock HS2024", RED);
    char timestring[40];
    sprintf(timestring, "Time:  %02i:%02i:%02i", hour, min, sec);
    lcdDrawString(fx32L, 30,100,timestring,WHITE);
    sprintf(timestring, "Alarm: %02i:%02i:%02i", alarm_hour, alarm_min, alarm_sec);
    lcdDrawString(fx32L, 30,130,timestring,WHITE);
    lcdDrawString(fx16M, 10,305, "0:    | 1L:Al-OnOff | 2L:Alarm | 3L:Time", GREEN);
    uint8_t al_x = 10, al_y = 160;
    drawAlarmSymbol(al_x, al_y);
    if(alarmOn == true) {
        lcdDrawLine(al_x+20,al_y + 20, al_x + 80, al_y + 80, RED);
        lcdDrawLine(al_x+20+1,al_y + 20, al_x + 80, al_y + 80-1, RED);
        lcdDrawLine(al_x+20,al_y + 20+1, al_x + 80-1, al_y + 80, RED);
    }    
    const int clock_x = 375, clock_y = 160;
    const int radius = 95;

    //Draw Clock
    lcdDrawFillCircle(clock_x, clock_y, radius+5, GRAY);
    lcdDrawFillCircle(clock_x, clock_y, radius, WHITE);
    lcdDrawCircle(clock_x, clock_y, radius, BLACK);
    lcdDrawCircle(clock_x, clock_y, radius+5, WHITE);
    for(int i = 0; i < 354; i+=6) {
        float angle = M_PI / 180.0 * i;   
        int x1 = clock_x + (sin(angle)*radius);
        int x2 = clock_x + (sin(angle)*(radius-7));
        int y1 = clock_y + (cos(angle)*radius);
        int y2 = clock_y + (cos(angle)*(radius-7));
        if(i % 30 == 0) {
            lcdDrawRectAngle(x2,y2,3,14, i, BLACK);
            lcdDrawRectAngle(x2,y2,2,13, i, BLACK);
            lcdDrawRectAngle(x2,y2,1,12, i, BLACK);
            
        } else {
            lcdDrawLine(x1,y1,x2,y2,BLACK);
        }
    }
    //Draw Clock Hands
    int hour_hand_angle_deg = (int)((360 - ((hour % 12) * 30) - (min * 0.5)) + 90) % 360;
    float hour_hand_angle_rad = M_PI / 180.0 * (float)(hour_hand_angle_deg);
    int min_hand_angle_deg = (int)((360 - (min * 6)) + 90) % 360;
    float min_hand_angle_rad = M_PI / 180.0 * (float)(min_hand_angle_deg);
    int sec_hand_angle_deg = (int)((360 - (sec * 6)) + 90) % 360;
    float sec_hand_angle_rad = M_PI / 180.0 * (float)(sec_hand_angle_deg);
    ESP_LOGI(TAG, "Hands: h:%i - m:%i - s:%i", hour_hand_angle_deg, min_hand_angle_deg, sec_hand_angle_deg);
    int hand_x = clock_x + (cos(hour_hand_angle_rad)*(40/2));
    int hand_y = clock_y - (sin(hour_hand_angle_rad)*(40/2));
    lcdDrawRectAngle(hand_x, hand_y, 65, 4, hour_hand_angle_deg, BLACK);
    lcdDrawRectAngle(hand_x, hand_y, 64, 3, hour_hand_angle_deg, BLACK);
    lcdDrawRectAngle(hand_x, hand_y, 63, 2, hour_hand_angle_deg, BLACK);
    lcdDrawRectAngle(hand_x, hand_y, 62, 1, hour_hand_angle_deg, BLACK);
    hand_x = clock_x + (cos(min_hand_angle_rad)*(65/2));
    hand_y = clock_y - (sin(min_hand_angle_rad)*(65/2));
    lcdDrawRectAngle(hand_x, hand_y, 90, 4, min_hand_angle_deg, BLACK);
    lcdDrawRectAngle(hand_x, hand_y, 89, 3, min_hand_angle_deg, BLACK);
    lcdDrawRectAngle(hand_x, hand_y, 88, 2, min_hand_angle_deg, BLACK);
    lcdDrawRectAngle(hand_x, hand_y, 87, 1, min_hand_angle_deg, BLACK);

    hand_x = clock_x + (cos(sec_hand_angle_rad)*(55/2));
    hand_y = clock_y - (sin(sec_hand_angle_rad)*(55/2));
    lcdDrawRectAngle(hand_x, hand_y, 80, 2, sec_hand_angle_deg, RED);
    lcdDrawRectAngle(hand_x, hand_y, 79, 1, sec_hand_angle_deg, RED);

    hand_x = clock_x + (cos(sec_hand_angle_rad)*(70));
    hand_y = clock_y - (sin(sec_hand_angle_rad)*(70));
    lcdDrawFillCircle(hand_x, hand_y, 7, RED);
    lcdDrawFillCircle(clock_x, clock_y, 5, RED);
    

}

void showTimeSetScreen(timeselection_t selection) {
    lcdDrawString(fx32L, 30,30,"TimeSet", RED);
    char timestring[40];
    sprintf(timestring, "Time:  %02i:%02i:%02i", hour, min, sec);
    lcdDrawString(fx32L, 30,100,timestring,WHITE);
    int x1=142,y1=100-32,x2=x1+32,y2=y1+32;
    x1 += (selection * 48);
    x2 = x1 + 32;
    lcdDrawRect(x1,y1,x2,y2,RED);
}

void showAlarmSetScreen(timeselection_t selection) {
    lcdDrawString(fx32L, 30,30,"AlarmSet", RED);
    char timestring[40];
    sprintf(timestring, "Alarm: %02i:%02i:%02i", alarm_hour, alarm_min, alarm_sec);
    lcdDrawString(fx32L, 30,100,timestring,WHITE);
    int x1=142,y1=100-32,x2=x1+32,y2=y1+32;
    x1 += (selection * 48);
    x2 = x1 + 32;
    lcdDrawRect(x1,y1,x2,y2,RED);
}

void showAlarmScreen() {

}

timeselection_t selectionInc(timeselection_t selection) {
    if(selection >= TIMESELECT_SEC) {
        selection = TIMESELECT_HOUR;
    } else {
        selection++;
    }
    ESP_LOGI(TAG, "Increment Selection -> %i", (int)selection);
    return selection;
}
void timeInc(timeselection_t selection) {
    switch(selection) {
        case TIMESELECT_HOUR:
            if(hour >= 23) {
                hour = 0;
            } else {
                hour++;
            }
        break;
        case TIMESELECT_MIN:
            if(min >= 59) {
                min = 0;
            } else {
                min++;
            }
        break;
        case TIMESELECT_SEC:
            if(sec >= 59) {
                sec = 0;
            } else {
                sec++;
            }
        break;
    }
    ESP_LOGI(TAG, "Increment Time (selection: %i)", (int)selection);
}
void timeDec(uint8_t selection) {
    switch(selection) {
        case TIMESELECT_HOUR:
            if(hour > 0) {
                hour--;
            } else {
                hour = 23;
            }
        break;
        case TIMESELECT_MIN:
            if(min > 0) {
                min--;
            } else {
                min = 59;
            }
        break;
        case TIMESELECT_SEC:
            if(sec > 0) {
                sec--;
            } else {
                sec = 59;
            }
        break;
    }
    ESP_LOGI(TAG, "Decrement Time (selection: %i)", (int)selection);
}
void alarmInc(uint8_t selection) {
    switch(selection) {
        case TIMESELECT_HOUR:
            if(alarm_hour >= 23) {
                alarm_hour = 0;
            } else {
                alarm_hour++;
            }
        break;
        case TIMESELECT_MIN:
            if(alarm_min >= 59) {
                alarm_min = 0;
            } else {
                alarm_min++;
            }
        break;
        case TIMESELECT_SEC:
            if(alarm_sec >= 59) {
                alarm_sec = 0;
            } else {
                alarm_sec++;
            }
        break;
    }
    ESP_LOGI(TAG, "Increment Alarm (selection: %i)", (int)selection);
}
void alarmDec(uint8_t selection) {
    switch(selection) {
        case TIMESELECT_HOUR:
            if(alarm_hour > 0) {
                alarm_hour--;
            } else {
                alarm_hour = 23;
            }
        break;
        case TIMESELECT_MIN:
            if(alarm_min > 0) {
                alarm_min--;
            } else {
                alarm_min = 59;
            }
        break;
        case TIMESELECT_SEC:
            if(alarm_sec > 0) {
                alarm_sec--;
            } else {
                alarm_sec = 59;
            }
        break;
    }
    ESP_LOGI(TAG, "Decrement Alarm (selection: %i)", (int)selection);
}

enum {
    UIMODE_INIT,
    UIMODE_MAINSCREEN,
    UIMODE_TIMESET,
    UIMODE_ALARMSET,
    UIMODE_ALARM,
};

uint8_t mode = UIMODE_INIT;

void alarmclock_ui_task(void* param) {
    bool alarmOn = true;
    uint32_t delaycounter = 3;
    timeselection_t selection = TIMESELECT_HOUR;
    for(;;) {        
        EventBits_t buttonState = xEventGroupGetBits(ev_alarmclock_status);
        // ESP_LOGI(TAG, "TIMESET: buttonvalue: %.02X", (int)buttonState);
        xEventGroupClearBits(ev_alarmclock_status, EV_SCREENUPDATE_IDLE);
        switch(mode) {
            case UIMODE_INIT:
                if(delaycounter > 0) {
                    delaycounter--;
                } else {
                    mode = UIMODE_MAINSCREEN;
                    setTime(18, 15, 00);
                    setAlarmTime(21, 35, 00);
                    xEventGroupSetBits(ev_alarmclock_status, EV_TIMEUPDATE);
                    ESP_LOGI(TAG, "Change to MAINSCREEN");
                }
            break;
            case UIMODE_MAINSCREEN:
                lcdFillScreen(BLACK);
                if(buttonState & EV_SW3_LONG) {
                    mode = UIMODE_TIMESET;
                    selection = TIMESELECT_HOUR;
                    xEventGroupClearBits(ev_alarmclock_status, EV_TIMEUPDATE);
                    ESP_LOGI(TAG, "Change to TIMESET");
                }
                else if(buttonState & EV_SW2_LONG) {
                    mode = UIMODE_ALARMSET;
                    selection = TIMESELECT_HOUR;
                    xEventGroupClearBits(ev_alarmclock_status, EV_TIMEUPDATE);
                    ESP_LOGI(TAG, "Change to ALARMSET");
                }
                else if(buttonState & EV_SW1_LONG) {
                    alarmOn = !alarmOn;
                    ESP_LOGI(TAG, "Change to ENABLE/DISABLE Alarm");
                }
                showMainScreen(alarmOn);
            break;
            case UIMODE_TIMESET:
                lcdFillScreen(BLACK);
                if(buttonState & EV_SW3_LONG) {
                    setTime(hour,min,sec);
                    xEventGroupSetBits(ev_alarmclock_status, EV_TIMEUPDATE);
                    mode = UIMODE_MAINSCREEN;
                }
                else if(buttonState & EV_SW0_SHORT) {
                    selection = selectionInc(selection);
                }
                else if(buttonState & EV_SW1_SHORT) {
                    timeDec(selection);
                }
                else if(buttonState & EV_SW2_SHORT) {
                    timeInc(selection);
                }
                showTimeSetScreen(selection);
            break;
            case UIMODE_ALARMSET:
                lcdFillScreen(BLACK);
                if(buttonState & EV_SW3_LONG) {
                    xEventGroupSetBits(ev_alarmclock_status, EV_TIMEUPDATE);
                    mode = UIMODE_MAINSCREEN;
                }
                else if(buttonState & EV_SW0_SHORT) {
                    selection = selectionInc(selection);
                }
                else if(buttonState & EV_SW1_SHORT) {
                    alarmDec(selection);
                }
                else if(buttonState & EV_SW2_SHORT) {
                    alarmInc(selection);
                }
                showAlarmSetScreen(selection);
            break;
            case UIMODE_ALARM:
                lcdFillScreen(BLACK);
                if(buttonState & EV_SW3_LONG) {
                    mode = UIMODE_MAINSCREEN;
                }
                showAlarmScreen();
            break;
            default:
                mode = UIMODE_INIT;
            break;
        }
        xEventGroupClearBits(ev_alarmclock_status, EV_ALL_SWITCH);
        xEventGroupSetBits(ev_alarmclock_status, EV_SCREENUPDATE_IDLE);
        lcdUpdateVScreen();
        vTaskDelay(200/portTICK_PERIOD_MS);
    }
}
void alarmclock_button_task(void* param) {
    for(;;) {
        EventBits_t buttonpresses = 0;
        xEventGroupWaitBits(ev_alarmclock_status, EV_SCREENUPDATE_IDLE, false, true, portMAX_DELAY);
        if(button_get_state(SW0, false) == SHORT_PRESSED) {
            buttonpresses |= EV_SW0_SHORT;
            xEventGroupSetBits(ev_alarmclock_status, EV_SW0_SHORT);
            ESP_LOGI(TAG, "SW0 Short pressed");
        }
        if(button_get_state(SW1, false) == SHORT_PRESSED) {
            buttonpresses |= EV_SW1_SHORT;
            xEventGroupSetBits(ev_alarmclock_status, EV_SW1_SHORT);
            ESP_LOGI(TAG, "SW1 Short pressed");
        }
        if(button_get_state(SW2, false) == SHORT_PRESSED) {
            buttonpresses |= EV_SW2_SHORT;
            xEventGroupSetBits(ev_alarmclock_status, EV_SW2_SHORT);
            ESP_LOGI(TAG, "SW2 Short pressed");
        }
        if(button_get_state(SW3, false) == SHORT_PRESSED) {
            buttonpresses |= EV_SW3_SHORT;
            xEventGroupSetBits(ev_alarmclock_status, EV_SW3_SHORT);
            ESP_LOGI(TAG, "SW3 Short pressed");
        }
        if(button_get_state(SW0, true) == LONG_PRESSED) {
            buttonpresses |= EV_SW0_LONG;
            xEventGroupSetBits(ev_alarmclock_status, EV_SW0_LONG);
            ESP_LOGI(TAG, "SW0 Long pressed");
        }
        if(button_get_state(SW1, true) == LONG_PRESSED) {
            buttonpresses |= EV_SW1_LONG;
            xEventGroupSetBits(ev_alarmclock_status, EV_SW1_LONG);
            ESP_LOGI(TAG, "SW1 Long pressed");
        }
        if(button_get_state(SW2, true) == LONG_PRESSED) {
            buttonpresses |= EV_SW2_LONG;
            xEventGroupSetBits(ev_alarmclock_status, EV_SW2_LONG);
            ESP_LOGI(TAG, "SW2 Long pressed");
        }
        if(button_get_state(SW3, true) == LONG_PRESSED) {
            buttonpresses |= EV_SW3_LONG;
            xEventGroupSetBits(ev_alarmclock_status, EV_SW3_LONG);
            ESP_LOGI(TAG, "SW3 Long pressed");
        }
        xEventGroupSetBits(ev_alarmclock_status, buttonpresses);
        vTaskDelay(50/portTICK_PERIOD_MS);
    }
}

void app_main()
{
    //Initialize Eduboard2 BSP
    eduboard2_init();
    
    ev_alarmclock_status = xEventGroupCreate();
    //Create templateTask
    xTaskCreate(alarmclock_timing_task, "TimingTask", 2*2048, NULL, 10, NULL);
    xTaskCreate(alarmclock_ui_task, "UITask", 2*2048, NULL, 10, NULL);
    xTaskCreate(alarmclock_button_task, "ButtonTask", 2*2048, NULL, 10, NULL);
    for(;;) {
        vTaskDelay(10000/portTICK_PERIOD_MS);
        // ESP_LOGI(TAG, "Alarmclock running...");
    }
}