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
#include "music.h"
#include "wifihandler.h"
#include "timekeeper.h"

#include "math.h"

#define TAG "ALARMCLOCK_HS2024"

#define UPDATETIME_MS 100

#define TIMEMODE_RTC

// EventBit Defines
#define EV_SW0_SHORT            0x01 << 0
#define EV_SW1_SHORT            0x01 << 1
#define EV_SW2_SHORT            0x01 << 2
#define EV_SW3_SHORT            0x01 << 3
#define EV_SW0_LONG             0x01 << 4
#define EV_SW1_LONG             0x01 << 5
#define EV_SW2_LONG             0x01 << 6
#define EV_SW3_LONG             0x01 << 7
#define EV_ROTARY_BUTTON_SHORT  0x01 << 8
#define EV_ROTARY_BUTTON_LONG   0x01 << 9
#define EV_ALL_SWITCH           0x03FF

#define EV_TIMEUPDATE           0x01 << 10
#define EV_SCREENUPDATE_IDLE    0x01 << 11
#define EV_ALARM_ACTIVE         0x01 << 12
EventGroupHandle_t ev_alarmclock_status;

// Timeselection enumeration
typedef enum {
    TIMESELECT_HOUR = 0,
    TIMESELECT_MIN = 1,
    TIMESELECT_SEC = 2,
} timeselection_t;

// Storage for Time
uint8_t hour = 0;
uint8_t min = 0;
uint8_t sec = 0;

// Storage for Alarm-Time
uint8_t alarm_hour = 0;
uint8_t alarm_min  = 0;
uint8_t alarm_sec  = 0;

// Task for time tracking. 
// Uses RTC if configured
void alarmclock_timing_task(void* param) {
    for(;;) {
        // If EV_TIMEUPDATE Bit is cleared, time will not update. Nescessary for TimeSet Mode
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
            #endif
        } else {
            vTaskDelay(500/portTICK_PERIOD_MS);
        }
    }
}

// Set Time in time storage
// Uses RTC if configured
void setTime(uint8_t _hour, uint8_t _min, uint8_t _sec) {
    #ifdef TIMEMODE_RTC
    rtc_set_time(_hour, _min, _sec);
    #else   
    hour = _hour;
    min  = _min;
    sec  = _sec;
    #endif
}

// Set Alarm-Time in time storage
void setAlarmTime(uint8_t _hour, uint8_t _min, uint8_t _sec) {
    alarm_hour = _hour;
    alarm_min = _min;
    alarm_sec = _sec;
}

// Check if Time and Alarm-Time are equal
bool checkAlarmReached() {
    if((hour == alarm_hour) && (min == alarm_min) && (sec == alarm_sec)) {
        return true;
    }
    return false;
}

// Alarm ringtone
note_t melody[] = {
    {NOTE_A4, 50},  // A4 for 500ms
    {NOTE_B4, 50},  // B4 for 500ms
    {NOTE_C5, 50},  // C5 for 500ms
    {NOTE_D5, 50},  // D5 for 500ms
    {NOTE_E5, 50},  // E5 for 500ms
    {NOTE_F5, 50},  // F5 for 500ms
    {NOTE_G5, 50},  // G5 for 500ms
    {NOTE_A5, 50},  // A5 for 500ms
    {0, 0}           // End of melody, 0 frequency and zero length means end
};

// Alarm ringtone task.
// Plays ringtone when according eventbit is set
void alarmclock_ringer_task(void* param) {
    buzzer_set_volume(1);
    uint16_t melodypos = 0;
    
    for(;;) {
        if(xEventGroupGetBits(ev_alarmclock_status) & EV_ALARM_ACTIVE) {
            if(melody[melodypos].freq > 0) {
                buzzer_start(melody[melodypos].freq, melody[melodypos].length_ms/portTICK_PERIOD_MS); //Play tone from melody for amount of time
                vTaskDelay(melody[melodypos].length_ms/portTICK_PERIOD_MS);
                melodypos++;
            } else {
                buzzer_stop();
                if(melody[melodypos].length_ms > 0) {
                    vTaskDelay(melody[melodypos].length_ms/portTICK_PERIOD_MS); //Break in melody
                    melodypos++;
                } else {
                    melodypos = 0;
                    vTaskDelay(1000/portTICK_PERIOD_MS); //Break between runs
                }
            }
        } else {
            buzzer_stop();
            melodypos = 0;
            vTaskDelay(100/portTICK_PERIOD_MS);
        }

    }
}

// Draw Function for Alarm-Symbol at position
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

// Draw Function for Clock Hands at position. 
// Some Fun with Trigonometry!
void drawClockHands(int x, int y, uint8_t hand_hour, uint8_t hand_min, uint8_t hand_sec) {
    int clock_x = 375;
    int clock_y = 160;
    clock_x = (x=0?x:clock_x);
    clock_y = (y=0?y:clock_y);

    //Draw Clock Hands
    int hour_hand_angle_deg = (int)((360 - ((hand_hour % 12) * 30) - (hand_min * 0.5)) + 90) % 360;
    float hour_hand_angle_rad = M_PI / 180.0 * (float)(hour_hand_angle_deg);
    int min_hand_angle_deg = (int)((360 - (hand_min * 6)) + 90) % 360;
    float min_hand_angle_rad = M_PI / 180.0 * (float)(min_hand_angle_deg);
    int sec_hand_angle_deg = (int)((360 - (hand_sec * 6)) + 90) % 360;
    float sec_hand_angle_rad = M_PI / 180.0 * (float)(sec_hand_angle_deg);
    // ESP_LOGI(TAG, "Hands: h:%i - m:%i - s:%i", hour_hand_angle_deg, min_hand_angle_deg, sec_hand_angle_deg);
    // Draw Hour Hand
    int hand_x = clock_x + (cos(hour_hand_angle_rad)*(40/2));
    int hand_y = clock_y - (sin(hour_hand_angle_rad)*(40/2));
    lcdDrawRectAngle(hand_x, hand_y, 65, 4, hour_hand_angle_deg, BLACK);
    lcdDrawRectAngle(hand_x, hand_y, 64, 3, hour_hand_angle_deg, BLACK);
    lcdDrawRectAngle(hand_x, hand_y, 63, 2, hour_hand_angle_deg, BLACK);
    lcdDrawRectAngle(hand_x, hand_y, 62, 1, hour_hand_angle_deg, BLACK);
    
    // Draw Minute Hand
    hand_x = clock_x + (cos(min_hand_angle_rad)*(65/2));
    hand_y = clock_y - (sin(min_hand_angle_rad)*(65/2));
    lcdDrawRectAngle(hand_x, hand_y, 90, 4, min_hand_angle_deg, BLACK);
    lcdDrawRectAngle(hand_x, hand_y, 89, 3, min_hand_angle_deg, BLACK);
    lcdDrawRectAngle(hand_x, hand_y, 88, 2, min_hand_angle_deg, BLACK);
    lcdDrawRectAngle(hand_x, hand_y, 87, 1, min_hand_angle_deg, BLACK);

    // Draw Second Hand
    hand_x = clock_x + (cos(sec_hand_angle_rad)*(55/2));
    hand_y = clock_y - (sin(sec_hand_angle_rad)*(55/2));
    lcdDrawRectAngle(hand_x, hand_y, 80, 2, sec_hand_angle_deg, RED);
    lcdDrawRectAngle(hand_x, hand_y, 79, 1, sec_hand_angle_deg, RED);
    hand_x = clock_x + (cos(sec_hand_angle_rad)*(70));
    hand_y = clock_y - (sin(sec_hand_angle_rad)*(70));
    lcdDrawFillCircle(hand_x, hand_y, 7, RED);
    lcdDrawFillCircle(clock_x, clock_y, 5, RED);
}

// Draw Clock Body at position
void drawClock(int x, int y) {
    int clock_x = 375;
    int clock_y = 160;
    int radius = 95;
    clock_x = (x=0?x:clock_x);
    clock_y = (y=0?y:clock_y);

    //Draw Clock
    lcdDrawFillCircle(clock_x, clock_y, radius+5, GRAY);
    lcdDrawFillCircle(clock_x, clock_y, radius, WHITE);
    lcdDrawCircle(clock_x, clock_y, radius, BLACK);
    lcdDrawCircle(clock_x, clock_y, radius+5, WHITE);
    // Calculate Minute and 5-Minute lines and draw them around the perimeter
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
}

// Draw Button-Description
void drawButtonDescription(char* bt0s, char* bt0l, char* bt1s, char* bt1l, char* bt2s, char* bt2l, char* bt3s, char* bt3l) {
    char bt0s_text[20];
    char bt0l_text[20];
    char bt1s_text[20];
    char bt1l_text[20];
    char bt2s_text[20];
    char bt2l_text[20];
    char bt3s_text[20];
    char bt3l_text[20];

    // Prepare all strings
    sprintf(bt0s_text, "S: %s", bt0s);
    sprintf(bt0l_text, "L: %s", bt0l);
    sprintf(bt1s_text, "S: %s", bt1s);
    sprintf(bt1l_text, "L: %s", bt1l);
    sprintf(bt2s_text, "S: %s", bt2s);
    sprintf(bt2l_text, "L: %s", bt2l);
    sprintf(bt3s_text, "S: %s", bt3s);
    sprintf(bt3l_text, "L: %s", bt3l);

    // Draw Button Boxes
    lcdDrawFillRect(0, 270, 479, 319, GRAY);
    for(int i = 0; i < 4; i++) {
        lcdDrawRect((i*120) + 1, 271, (i*120) + 118, 478, BLACK);
    }

    // Draw Button-Text
    lcdDrawString(fx16M, 0*120+10, 293, bt0s_text, WHITE);
    lcdDrawString(fx16M, 0*120+10, 313, bt0l_text, WHITE);
    lcdDrawString(fx16M, 1*120+10, 293, bt1s_text, WHITE);
    lcdDrawString(fx16M, 1*120+10, 313, bt1l_text, WHITE);
    lcdDrawString(fx16M, 2*120+10, 293, bt2s_text, WHITE);
    lcdDrawString(fx16M, 2*120+10, 313, bt2l_text, WHITE);
    lcdDrawString(fx16M, 3*120+10, 293, bt3s_text, WHITE);
    lcdDrawString(fx16M, 3*120+10, 313, bt3l_text, WHITE);
}

// Main-Screen Draw Function.
void showMainScreen(bool alarmOn) {
    lcdDrawString(fx32L, 30,30,"AlarmClock HS2024", RED);
    char timestring[40];
    sprintf(timestring, "Time:  %02i:%02i:%02i", hour, min, sec);
    lcdDrawString(fx32L, 30,100,timestring,WHITE);
    sprintf(timestring, "Alarm: %02i:%02i:%02i", alarm_hour, alarm_min, alarm_sec);
    lcdDrawString(fx32L, 30,130,timestring,WHITE);
    drawButtonDescription(  "-",            "-",
                            "-",            "Alarm 1/0",
                            "-",            "AL-Set",
                            "-",            "TM-Set");
    uint8_t al_x = 10, al_y = 160;
    drawAlarmSymbol(al_x, al_y);
    if(alarmOn == false) {
        lcdDrawLine(al_x+20,al_y + 20, al_x + 80, al_y + 80, RED);
        lcdDrawLine(al_x+20+1,al_y + 20, al_x + 80, al_y + 80-1, RED);
        lcdDrawLine(al_x+20,al_y + 20+1, al_x + 80-1, al_y + 80, RED);
    }
    drawClock(-1, -1);
    drawClockHands(-1, -1, hour, min, sec);
}

// TimeSet-Screen Draw Function.
void showTimeSetScreen(timeselection_t selection) {
    lcdDrawString(fx32L, 30,30,"TimeSet", RED);
    char timestring[40];
    sprintf(timestring, "Time:  %02i:%02i:%02i", hour, min, sec);
    lcdDrawString(fx32L, 30,100,timestring,WHITE);
    int x1=142,y1=100-32,x2=x1+32,y2=y1+32;
    x1 += (selection * 48);
    x2 = x1 + 32;
    lcdDrawRect(x1,y1,x2,y2,RED);
    drawClock(-1, -1);
    drawClockHands(-1, -1, hour, min, sec);
    drawButtonDescription(  "Select",       "-",
                            "Dec",          "-",
                            "Inc",          "-",
                            "-",            "back");
}

// AlarmSet-Screen Draw Function.
void showAlarmSetScreen(timeselection_t selection) {
    lcdDrawString(fx32L, 30,30,"AlarmSet", RED);
    char timestring[40];
    sprintf(timestring, "Alarm: %02i:%02i:%02i", alarm_hour, alarm_min, alarm_sec);
    lcdDrawString(fx32L, 30,100,timestring,WHITE);
    int x1=142,y1=100-32,x2=x1+32,y2=y1+32;
    x1 += (selection * 48);
    x2 = x1 + 32;
    lcdDrawRect(x1,y1,x2,y2,RED);
    drawClock(-1, -1);
    drawClockHands(-1, -1, alarm_hour, alarm_min, alarm_sec);
    drawButtonDescription(  "Select",       "-",
                            "Dec",          "-",
                            "Inc",          "-",
                            "-",            "back");
}

// Alarm-Screen Draw Function.
void showAlarmScreen() {
    pixel_jpeg **pixels;
	uint16_t imageWidth;
	uint16_t imageHeight;
	uint16_t width = lcdGetWidth();
	uint16_t height = lcdGetHeight();
	char file[32];
	strcpy(file, "/spiffs/grumpycat.jpg");
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
    // drawButtonDescription(  "-",            "-",
    //                         "-",            "-",
    //                         "-",            "-",
    //                         "-",            "back");
}

// Increment Time-Selection and automatically overflow to 0 when last one is reached
timeselection_t selectionInc(timeselection_t selection) {
    if(selection >= TIMESELECT_SEC) {
        selection = TIMESELECT_HOUR;
    } else {
        selection++;
    }
    ESP_LOGI(TAG, "Increment Selection -> %i", (int)selection);
    return selection;
}

// Change Time storage by selection and amount. (Positive and Negative Numbers possible)
void timeChange(uint8_t selection, int16_t amount) {    
    switch(selection) {
        case TIMESELECT_HOUR:
            if(amount > 0) {
                for(int i = 0; i < amount; i++) {
                    if(hour >= 23) {
                        hour = 0;
                    } else {
                        hour++;
                    }
                }
            } else if(amount < 0) {
                for(int i = 0; i > amount; i--) {
                    if(hour > 0) {
                        hour--;
                    } else {
                        hour = 23;
                    }
                }
            }
        break;
        case TIMESELECT_MIN:
            if(amount > 0) {
                for(int i = 0; i < amount; i++) {
                    if(min >= 59) {
                        min = 0;
                    } else {
                        min++;
                    }
                }
            } else if(amount < 0) {
                for(int i = 0; i > amount; i--) {
                    if(min > 0) {
                        min--;
                    } else {
                        min = 59;
                    }
                }
            }
        break;
        case TIMESELECT_SEC:
            if(amount > 0) {
                for(int i = 0; i < amount; i++) {
                    if(sec >= 59) {
                        sec = 0;
                    } else {
                        sec++;
                    }
                }
            } else if(amount < 0) {
                for(int i = 0; i > amount; i--) {
                    if(sec > 0) {
                        sec--;
                    } else {
                        sec = 59;
                    }
                }
            }
        break;
    }
    ESP_LOGI(TAG, "Time Change (selection: %i)", (int)selection);
}

// Change Alarm-Time storage by selection and amount. (Positive and Negative Numbers possible)
void alarmChange(uint8_t selection, int16_t amount) {    
    switch(selection) {
        case TIMESELECT_HOUR:
            if(amount > 0) {
                for(int i = 0; i < amount; i++) {
                    if(alarm_hour >= 23) {
                        alarm_hour = 0;
                    } else {
                        alarm_hour++;
                    }
                }
            } else if(amount < 0) {
                for(int i = 0; i > amount; i--) {
                    if(alarm_hour > 0) {
                        alarm_hour--;
                    } else {
                        alarm_hour = 23;
                    }
                }
            }
        break;
        case TIMESELECT_MIN:
            if(amount > 0) {
                for(int i = 0; i < amount; i++) {
                    if(alarm_min >= 59) {
                        alarm_min = 0;
                    } else {
                        alarm_min++;
                    }
                }
            } else if(amount < 0) {
                for(int i = 0; i > amount; i--) {
                    if(alarm_min > 0) {
                        alarm_min--;
                    } else {
                        alarm_min = 59;
                    }
                }
            }
        break;
        case TIMESELECT_SEC:
            if(amount > 0) {
                for(int i = 0; i < amount; i++) {
                    if(alarm_sec >= 59) {
                        alarm_sec = 0;
                    } else {
                        alarm_sec++;
                    }
                }
            } else if(amount < 0) {
                for(int i = 0; i > amount; i--) {
                    if(alarm_sec > 0) {
                        alarm_sec--;
                    } else {
                        alarm_sec = 59;
                    }
                }
            }
        break;
    }
    ESP_LOGI(TAG, "Time Change (selection: %i)", (int)selection);
}

// Menu-Mode Enumeration
enum {
    UIMODE_INIT,
    UIMODE_MAINSCREEN,
    UIMODE_TIMESET,
    UIMODE_ALARMSET,
    UIMODE_ALARM,
};

// Main UI Task. 
// Handles state machine and behaviour of UI.
void alarmclock_ui_task(void* param) {
    uint8_t mode = UIMODE_INIT;
    bool alarmOn = false;
    uint32_t delaycounter = 3;
    timeselection_t selection = TIMESELECT_HOUR;
    for(;;) {        
        EventBits_t buttonState = xEventGroupGetBits(ev_alarmclock_status);
        // ESP_LOGI(TAG, "TIMESET: buttonvalue: %.02X", (int)buttonState);
        xEventGroupClearBits(ev_alarmclock_status, EV_SCREENUPDATE_IDLE);
        int32_t rotationamount = rotary_encoder_get_rotation(true);
        switch(mode) {
            case UIMODE_INIT:
                if(delaycounter > 0) {
                    delaycounter--;
                } else {
                    mode = UIMODE_MAINSCREEN;
                    #ifdef TIMEMODE_RTC
                    uint8_t h,m,s;
                    rtc_get_time(&h,&m,&s);
                    if(h > 0 || m > 0 || s > 3) {
                        ESP_LOGI(TAG, "Time allready running. Do not initialize Time");
                    } else {
                        setTime(18, 15, 00);
                    }
                    #else
                    setTime(18, 15, 00);
                    #endif
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
                else if(buttonState & EV_SW0_LONG) {
                    mode = UIMODE_ALARM;
                }
                if((checkAlarmReached() == true) && (alarmOn == true)) {
                    mode = UIMODE_ALARM;
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
                else if(buttonState & EV_ROTARY_BUTTON_SHORT) {
                    selection = selectionInc(selection);
                }
                else if(buttonState & EV_SW1_SHORT) {
                    timeChange(selection, -1);
                }
                else if(buttonState & EV_SW2_SHORT) {
                    timeChange(selection, 1);
                }
                if(rotationamount != 0) {
                    // rotationamount = (selection==TIMESELECT_HOUR?rotationamount:rotationamount);
                    timeChange(selection, rotationamount);
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
                else if(buttonState & EV_ROTARY_BUTTON_SHORT) {
                    selection = selectionInc(selection);
                }
                else if(buttonState & EV_SW1_SHORT) {
                    alarmChange(selection, -1);
                }
                else if(buttonState & EV_SW2_SHORT) {
                    alarmChange(selection, 1);
                }
                if(rotationamount != 0) {
                    // rotationamount = (selection==TIMESELECT_HOUR?rotationamount/4:rotationamount);
                    alarmChange(selection, rotationamount);
                }
                showAlarmSetScreen(selection);
            break;
            case UIMODE_ALARM:
                lcdFillScreen(BLACK);
                xEventGroupSetBits(ev_alarmclock_status, EV_ALARM_ACTIVE);
                if(buttonState & EV_SW3_LONG) {
                    xEventGroupClearBits(ev_alarmclock_status, EV_ALARM_ACTIVE);
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
        if(button_get_state(SW0, false) == SHORT_PRESSED) {
            buttonpresses |= EV_SW0_SHORT;
            ESP_LOGI(TAG, "SW0 Short pressed");
        }
        if(button_get_state(SW1, false) == SHORT_PRESSED) {
            buttonpresses |= EV_SW1_SHORT;
            ESP_LOGI(TAG, "SW1 Short pressed");
        }
        if(button_get_state(SW2, false) == SHORT_PRESSED) {
            buttonpresses |= EV_SW2_SHORT;
            ESP_LOGI(TAG, "SW2 Short pressed");
        }
        if(button_get_state(SW3, false) == SHORT_PRESSED) {
            buttonpresses |= EV_SW3_SHORT;
            ESP_LOGI(TAG, "SW3 Short pressed");
        }
        if(button_get_state(SW0, true) == LONG_PRESSED) {
            buttonpresses |= EV_SW0_LONG;
            ESP_LOGI(TAG, "SW0 Long pressed");
        }
        if(button_get_state(SW1, true) == LONG_PRESSED) {
            buttonpresses |= EV_SW1_LONG;
            ESP_LOGI(TAG, "SW1 Long pressed");
        }
        if(button_get_state(SW2, true) == LONG_PRESSED) {
            buttonpresses |= EV_SW2_LONG;
            ESP_LOGI(TAG, "SW2 Long pressed");
        }
        if(button_get_state(SW3, true) == LONG_PRESSED) {
            buttonpresses |= EV_SW3_LONG;
            ESP_LOGI(TAG, "SW3 Long pressed");
        }
        if(rotary_encoder_button_get_state(true) == SHORT_PRESSED) {
            buttonpresses |= EV_ROTARY_BUTTON_SHORT;
            ESP_LOGI(TAG, "Rotary Button Short pressed");
        }
        if(rotary_encoder_button_get_state(true) == LONG_PRESSED) {
            buttonpresses |= EV_ROTARY_BUTTON_LONG;
            ESP_LOGI(TAG, "Rotary Button Long pressed");
        }
        xEventGroupWaitBits(ev_alarmclock_status, EV_SCREENUPDATE_IDLE, false, true, portMAX_DELAY);
        xEventGroupSetBits(ev_alarmclock_status, buttonpresses);
        vTaskDelay(50/portTICK_PERIOD_MS);
    }
}

// application entry
void app_main()
{
    //Initialize Eduboard2 BSP
    eduboard2_init();

    // Memory Monitor Tool
    // initMemon();
    // memon_enable();
    // memon_setUpdateTime(3);

    //Initialize Wifihandler to connect to network
    init_wifihandler();
    //Initialize timekeeper for handling sntp time update
    init_timekeeper();
    
    //Initialize main Eventgroup for Button and Status communication
    ev_alarmclock_status = xEventGroupCreate();

    //Start all Tasks
    xTaskCreate(alarmclock_timing_task, "TimingTask", 2*2048, NULL, 10, NULL);
    xTaskCreate(alarmclock_ui_task, "UITask", 2*2048, NULL, 10, NULL);
    xTaskCreate(alarmclock_button_task, "ButtonTask", 2*2048, NULL, 10, NULL);
    xTaskCreate(alarmclock_ringer_task, "RingerTask", 2*2048, NULL, 10, NULL);
    for(;;) {
        vTaskDelay(10000/portTICK_PERIOD_MS);
        // Check if SNTP Time is available
        if(timekeeper_TimeInitialized()) {
            systime_t timenow;
            timekeeper_GetTime(&timenow);
            // Calculate time deviation from SNTP Server to local time
            int deviation = ((timenow.time.hour - hour) % 24) * 3600;
            deviation += ((timenow.time.min - min) % 60) * 60;
            deviation += ((timenow.time.sec - sec) % 60);
            ESP_LOGI(TAG, "Time Deviation from NTP Server: %is" , deviation);
            // if time deviation is too big, set local time to SNTP-Time.
            if((deviation > 10) || (deviation < -10)) {
                ESP_LOGI(TAG, "Time Deviation from NTP too large. Set Time to %02i:%02i:%02i", timenow.time.hour, timenow.time.min, timenow.time.sec);
                setTime(timenow.time.hour, timenow.time.min, timenow.time.sec);
            }
        }
    }
}