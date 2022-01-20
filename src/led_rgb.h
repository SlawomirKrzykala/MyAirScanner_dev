/*
 * Copyright (c) 2021, 2022 by
 * Slawomir Krzykala. All rights reserved.
 */ 
#ifndef LED_RGB_H_  
#define LED_RGB_H_

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"

//CONFIG
#define LED_RGB_RED_GPIO    32
#define LED_RGB_GREEN_GPIO  14
#define LED_RGB_BLUE_GPIO   25

#define LED_RGB_PWM_FREQ        6000
#define LED_RGB_PWM_MAX_DUTY    255
#define LED_RGB_FADE_TIME       600
    
typedef enum {
    LED_RGB_OK                  = 0,
    LED_RGB_FAIL_INIT_TIMER     = -1,
    LED_RGB_FAIL_INIT_CHANNEL   = -2,
    LED_RGB_FAIL_INIT_FADE      = -3,
    LED_RGB_FAIL_SET            = -4
} led_rgb_error_t;

led_rgb_error_t led_rgb_init(void);
led_rgb_error_t led_rgb_test(void);
led_rgb_error_t led_rgb_set(uint8_t r, uint8_t g, uint8_t b);

#endif
