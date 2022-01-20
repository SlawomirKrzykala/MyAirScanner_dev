/*
 * Copyright (c) 2021, 2022 by
 * Slawomir Krzykala. All rights reserved.
 */ 
#ifndef PMS_H_  
#define PMS_H_

#include <stdio.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

//CONFIG
#define PMS_RESET_GPIO  5
#define PMS_SET_GPIO    2
#define PMS_RX_GPIO     16
#define PMS_TX_GPIO     17

#define PMS_UART_NUM                UART_NUM_2
#define PMS_UART_BUFFER_RX_SIZE     256
#define PMS_UART_BUFFER_TX_SIZE     256

//ERROR
typedef enum {
    PMS_OK                 = 0,
    PMS_LOW_DATA_BUFOR     = -1,
    PMS_NOT_FULL_FRAME     = -2,
    PMS_NOT_FIND_FRAME     = -3,
    PMS_BAD_CHECKSUM       = -4,
    PMS_FAIL_SEND_FRAME    = -5,
    PMS_FAIL_INIT_GPIO     = -6,
    PMS_FAIL_INIT_UART     = -7,
    PMS_FAIL_SET_LEVEL_GPIO = -8,
    PMS_BAD_AVG_ARR_SIZE    = -9,
    PMS_FAIL_MEMALLOC       = -10
} pms_error_t;

typedef enum {
    PMS_WORKMODE_PASSIVE = 0,   
    PMS_WORKMODE_ACTIVE  = 1
} pms_workmode_t;

struct PMS_SizePM { //concentration unit μ g/m3
    uint_least16_t pm10;
    uint_least16_t pm25;
    uint_least16_t pm100;
};

struct PMS_Num { //number of particles with diameter beyond
    uint_least16_t um3;
    uint_least16_t um5;
    uint_least16_t um10;
    uint_least16_t um25;
    uint_least16_t um50;
    uint_least16_t um100;
};

typedef struct{  // all measurements
    struct PMS_SizePM sm;
    struct PMS_SizePM ae;
    struct PMS_Num num;
} pms_measurement_t ;



pms_error_t pms_init(const pms_workmode_t workmode);
pms_error_t pms_wake(void);
pms_error_t pms_sleep(void);
pms_error_t pms_set_workmode(const pms_workmode_t mode);
pms_error_t pms_request_read(pms_measurement_t *pms_value);
pms_error_t pms_read_from_buffer(pms_measurement_t *pms_value);
pms_error_t pms_calc_avg(const pms_measurement_t *arr_src, pms_measurement_t *dst, uint8_t arr_size);

#endif