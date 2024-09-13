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

#define TAG "TEMPLATE"

#define UPDATETIME_MS 100

#define TIMEMODE_RTC


#define EV_SW0_SHORT    1 << 0
#define EV_SW1_SHORT    1 << 1
#define EV_SW2_SHORT    1 << 2
#define EV_SW3_SHORT    1 << 3
#define EV_SW0_LONG     1 << 4
#define EV_SW1_LONG     1 << 5
#define EV_SW2_LONG     1 << 6
#define EV_SW3_LONG     1 << 7
#define EV_ALL_SWITCH   0xFF
EventGroupHandle_t ev_alarmclock_status;

uint8_t hour = 0;
uint8_t min = 0;
uint8_t sec = 0;

uint8_t alarm_hour = 0;
uint8_t alarm_min  = 0;
uint8_t alarm_sec  = 0;

void alarmclock_timing_task(void* param) {

    for(;;) {
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
        #endif
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
    lcdDrawString(fx32M, 30,190,timestring,WHITE);
    sprintf(timestring, "Alarm: %02i:%02i:%02i", alarm_hour, alarm_min, alarm_sec);
    lcdDrawString(fx32M, 120,260,timestring,WHITE);
    drawAlarmSymbol(10, 200);
    if(alarmOn == true) {
        lcdDrawLine(30,220, 80, 280, RED);
        lcdDrawLine(31,220, 80, 279, RED);
        lcdDrawLine(30,221, 79, 280, RED);
    }
}

void showTimeSetScreen() {

}

void showAlarmSetScreen() {

}

void showAlarmScreen() {

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
    for(;;) {
        lcdFillScreen(BLACK);
        EventBits_t state = xEventGroupGetBits(ev_alarmclock_status);
        switch(mode) {
            case UIMODE_INIT:
                setTime(18, 15, 00);
                setAlarmTime(21, 35, 00);
                if(delaycounter > 0) {
                    delaycounter--;
                } else {
                    mode = UIMODE_MAINSCREEN;
                }
            break;
            case UIMODE_MAINSCREEN:
                showMainScreen(alarmOn);
            break;
            case UIMODE_TIMESET:
                showTimeSetScreen();
            break;
            case UIMODE_ALARMSET:
                showAlarmSetScreen();
            break;
            case UIMODE_ALARM:
                showAlarmScreen();
            break;
            default:
                mode = UIMODE_INIT;
            break;
        }

        
        
        


        lcdUpdateVScreen();
        xEventGroupClearBits(ev_alarmclock_status, EV_ALL_SWITCH);
        vTaskDelay(200/portTICK_PERIOD_MS);
    }
}
void alarmclock_button_task(void* param) {

    for(;;) {
        xEventGroupSetBits(ev_alarmclock_status, EV_SW0_SHORT);
        if(button_get_state(SW0, false) == SHORT_PRESSED) {
            xEventGroupSetBits(ev_alarmclock_status, EV_SW0_SHORT);
        }
        if(button_get_state(SW1, false) == SHORT_PRESSED) {
            xEventGroupSetBits(ev_alarmclock_status, EV_SW1_SHORT);
        }
        if(button_get_state(SW2, false) == SHORT_PRESSED) {
            xEventGroupSetBits(ev_alarmclock_status, EV_SW2_SHORT);
        }
        if(button_get_state(SW3, false) == SHORT_PRESSED) {
            xEventGroupSetBits(ev_alarmclock_status, EV_SW3_SHORT);
        }
        if(button_get_state(SW0, true) == LONG_PRESSED) {
            xEventGroupSetBits(ev_alarmclock_status, EV_SW0_LONG);
        }
        if(button_get_state(SW1, true) == LONG_PRESSED) {
            xEventGroupSetBits(ev_alarmclock_status, EV_SW1_LONG);
        }
        if(button_get_state(SW2, true) == LONG_PRESSED) {
            xEventGroupSetBits(ev_alarmclock_status, EV_SW2_LONG);
        }
        if(button_get_state(SW3, true) == LONG_PRESSED) {
            xEventGroupSetBits(ev_alarmclock_status, EV_SW3_LONG);
        }
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
    xTaskCreate(alarmclock_ui_task, "ButtonTask", 2*2048, NULL, 10, NULL);
    xTaskCreate(alarmclock_button_task, "ButtonTask", 2*2048, NULL, 10, NULL);
    for(;;) {
        vTaskDelay(10000/portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Alarmclock running...");
    }
}