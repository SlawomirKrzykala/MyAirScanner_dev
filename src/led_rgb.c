/*
 * Copyright (c) 2021, 2022 by
 * Slawomir Krzykala. All rights reserved.
 */ 
#include "led_rgb.h"

static const char *TAG = "LED_RGB";

    static const ledc_timer_config_t LED_RGB_TIMER_CFG = {
        .duty_resolution = LEDC_TIMER_8_BIT,    
        .freq_hz = LED_RGB_PWM_FREQ,                
        .speed_mode = LEDC_HIGH_SPEED_MODE,     
        .timer_num = LEDC_TIMER_0,              
        .clk_cfg = LEDC_AUTO_CLK,               
    };

    static const ledc_channel_config_t LED_RGB_RED_CHANNEL_CFG = {
        .channel    = LEDC_CHANNEL_0,
        .duty       = 0,
        .gpio_num   = LED_RGB_RED_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_0,
    };

    static const ledc_channel_config_t LED_RGB_GREEN_CHANNEL_CFG = {
        .channel    = LEDC_CHANNEL_1,
        .duty       = 0,
        .gpio_num   = LED_RGB_GREEN_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_0,
    };

    static const ledc_channel_config_t LED_RGB_BLUE_CHANNEL_CFG = {
        .channel    = LEDC_CHANNEL_3,
        .duty       = 0,
        .gpio_num   = LED_RGB_BLUE_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_0,
    };
    


    led_rgb_error_t led_rgb_init(void)
    {
        if(ledc_timer_config(&LED_RGB_TIMER_CFG) !=0)
        {
            ESP_LOGE(TAG, "Fail init timer");
            return LED_RGB_FAIL_INIT_TIMER;
        }

        if(ledc_channel_config(&LED_RGB_RED_CHANNEL_CFG) !=0 ||
            ledc_channel_config(&LED_RGB_GREEN_CHANNEL_CFG) !=0 ||
            ledc_channel_config(&LED_RGB_BLUE_CHANNEL_CFG) !=0)
        {
            ESP_LOGE(TAG, "Fail init channel");
            return LED_RGB_FAIL_INIT_CHANNEL;
        }

        if(ledc_fade_func_install(0) !=0)
        {
            ESP_LOGE(TAG, "Fail init fade func");
            return LED_RGB_FAIL_INIT_FADE;           
        }

        return LED_RGB_OK;
    }

    led_rgb_error_t led_rgb_test(void){
        //red
        if(led_rgb_set(255, 0, 0)!=0)
        {
            ESP_LOGE(TAG, "Fail set fade 0/red during test procedure.");
            return LED_RGB_FAIL_SET;
        }
        vTaskDelay(LED_RGB_FADE_TIME / portTICK_PERIOD_MS);

        //green
        if(led_rgb_set(0, 255, 0)!=0)
        {            
            ESP_LOGE(TAG, "Fail set fade red/green during test procedure.");
            return LED_RGB_FAIL_SET;
        }
        vTaskDelay(LED_RGB_FADE_TIME / portTICK_PERIOD_MS);

        //blue
        if(led_rgb_set(0, 0, 255)!=0)
        {            
            ESP_LOGE(TAG, "Fail set fade green/blue during test procedure.");
            return LED_RGB_FAIL_SET;
        }
        vTaskDelay(LED_RGB_FADE_TIME / portTICK_PERIOD_MS);

        //nothing
        if(led_rgb_set(0, 0, 0)!=0)
        {
            ESP_LOGE(TAG, "Fail set fade blue/0 during test procedure.");
            return LED_RGB_FAIL_SET;
        }

        return LED_RGB_OK;
    }

    led_rgb_error_t led_rgb_set(uint8_t r, uint8_t g, uint8_t b)
    {
        if(ledc_set_fade_time_and_start(LED_RGB_RED_CHANNEL_CFG.speed_mode, LED_RGB_RED_CHANNEL_CFG.channel, r, LED_RGB_FADE_TIME, LEDC_FADE_NO_WAIT) !=0 ||
            ledc_set_fade_time_and_start(LED_RGB_GREEN_CHANNEL_CFG.speed_mode, LED_RGB_GREEN_CHANNEL_CFG.channel, g, LED_RGB_FADE_TIME, LEDC_FADE_NO_WAIT) !=0 ||
            ledc_set_fade_time_and_start(LED_RGB_BLUE_CHANNEL_CFG.speed_mode, LED_RGB_BLUE_CHANNEL_CFG.channel, b, LED_RGB_FADE_TIME, LEDC_FADE_NO_WAIT) !=0)
        {
            ESP_LOGE(TAG, "Fail set fade rgb during test procedure.");
            return LED_RGB_FAIL_SET;
        }
        return LED_RGB_OK;
    }


