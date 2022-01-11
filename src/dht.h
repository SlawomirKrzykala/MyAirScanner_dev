// Copyright 2021-2022 Slawomir Krzykala
#ifndef DHT_H_  
#define DHT_H_

#include "esp_log.h"
#include "driver/gpio.h"


//CONFIG
#define DHT_DATA_GPIO   23
#define DHT_VCC_GPIO    22

//ERROR
typedef enum {
    DHT_OK                      = 0,
    DHT_TIMEOUT_START_TRANS     = -1,
    DHT_TIMEOUT_RECEIVE_DATA    = -2,
    DHT_BAD_CHECKSUM            = -3,
    DHT_FAIL_INIT               = -4,
    DHT_BAD_AVG_ARR_SIZE        = -5
} dht_error_t;


typedef struct {
    int_least16_t temperature;
    uint_least16_t humidity;
} dht_measurement_t;


dht_error_t dht_init(void);
dht_error_t dht_read(dht_measurement_t *dst);
dht_error_t dht_calc_avg(const dht_measurement_t *arr_src, dht_measurement_t *dst, uint8_t arr_size);

//static function
    //static inline void get_state_time_us(const int state, const int_fast16_t usTimeOut, int_fast16_t *usTime);

#endif
