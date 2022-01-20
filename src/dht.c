/*
 * Copyright (c) 2021, 2022 by
 * Slawomir Krzykala. All rights reserved.
 */ 
#include "dht.h"

static const char *TAG = "DHT";

static inline void dht_get_state_time_us(const int state, const int_fast16_t usTimeOut, int_fast16_t *usTime);



dht_error_t dht_init(void) //GPIO VCC DHT set high state
{
    if(gpio_reset_pin(DHT_VCC_GPIO)  !=0 ||
        gpio_set_direction(DHT_VCC_GPIO, GPIO_MODE_OUTPUT)  !=0 ||
        gpio_set_level(DHT_VCC_GPIO, 1) !=0)
    {
        return DHT_FAIL_INIT;
        ESP_LOGE(TAG, "Fail init GPIO VCC");
    }

    return DHT_OK;
}

static inline void dht_get_state_time_us(const int state, const int_fast16_t usTimeOut, int_fast16_t *usTime)
{
    *usTime = 0;
    while (gpio_get_level(DHT_DATA_GPIO) == state)
    {
        ets_delay_us(1);
        if ((++(*usTime)) > usTimeOut)
        {
            *usTime=-1;
            return;
        }
    }
    return;
}

dht_error_t dht_read(dht_measurement_t *dst)//read temp and hum
{
    int_fast16_t usTimeState = 0;
    uint_fast8_t received_data[5]={0};    // DHT send 5bytes

// send start signal to DHT
    
    if(//gpio_reset_pin(DHT_DATA_GPIO) !=0 ||
        gpio_set_direction(DHT_DATA_GPIO, GPIO_MODE_OUTPUT) != 0)
    {
        return DHT_FAIL_INIT;
        ESP_LOGE(TAG, "Fail init GPIO DATA");
    }
    // low state form 2ms (in doc -> 1-10ms)
    gpio_set_level(DHT_DATA_GPIO, 0);
    ets_delay_us(8600);	

    // high state for 26us (in doc -> 20-40us)
    gpio_set_level(DHT_DATA_GPIO, 1);
    ets_delay_us(26);


// wait for response DHT
    //gpio_reset_pin(DHT_DATA_GPIO);
    gpio_set_direction(DHT_DATA_GPIO, GPIO_MODE_INPUT);
    //gpio_set_pull_mode(DHT_DATA_GPIO, GPIO_FLOATING);

    // wait for high state (in doc -> low state 80us)
    dht_get_state_time_us(0, 260, &usTimeState);
    if(usTimeState < 0)
    {
        ESP_LOGE(TAG, "Timeout waiting for high state.");
        return DHT_TIMEOUT_START_TRANS;
    }  

    // wait for low signal (in doc -> high state 80us)
    dht_get_state_time_us(1, 260, &usTimeState);
    if(usTimeState < 0)
    {
        ESP_LOGE(TAG, "Timeout waiting for low state.");
        return DHT_TIMEOUT_START_TRANS;
    }  

// receive data from DHT
    for (uint_fast8_t nByte = 0; nByte < 5; ++nByte) // iteration for 5bytes data
    {
        for (int_fast8_t nBit = 7; nBit >= 0; --nBit) // iteration for 8bits every byte
        {
            // every bits is preceded by low state (in doc-> 50us)
            dht_get_state_time_us(0, 260, &usTimeState);
            if(usTimeState < 0)
            {
                ESP_LOGE(TAG, "Timeout waiting for high state during receiving data.");
                return DHT_TIMEOUT_RECEIVE_DATA;
            }  

            // in doc -> bit receive "0" if high state timme 26-28us, "1" if high state is 70us
            dht_get_state_time_us(1, 260, &usTimeState);
            if(usTimeState < 0)
            {
                ESP_LOGE(TAG, "Timeout waiting for low state during receiving data.");
                return DHT_TIMEOUT_RECEIVE_DATA;
            }

            // save received data, on start all bits are "0"
            // if receive "1", "1" is shift to nBit position and OR with nByte data
            // if receive "0" do nothing
            if (usTimeState > 46)
                received_data[nByte] |= (1 << (nBit));
        }
    }

    
// parse received data

    // check checksum 
    if (((uint8_t)(received_data[0] + received_data[1] + received_data[2] + received_data[3])) != received_data[4])
    {
        ESP_LOGE(TAG, "Bad checksum received data.");
        return DHT_BAD_CHECKSUM;
    }


    // receiveData[2] and receiveData[3] to temperature, in real this is temperature/10 
    dst->temperature = ((((uint16_t)(received_data[2]&0x7F))<<8) | ((uint16_t)received_data[3]));
    // if negative temp, firt bit is 1
    if(received_data[2] & 0x80)
        dst->temperature*=-1;


    // receiveData[0] and receiveData[1] to humidity, in real this is humidity/10 
    dst->humidity = ((((uint16_t)received_data[0])<<8) | ((uint16_t)received_data[1]));

    return DHT_OK;
}

dht_error_t dht_calc_avg(const dht_measurement_t *arr_src, dht_measurement_t *dst, uint8_t arr_size)//calc avg
{
    if(arr_size < 1)
    {
        ESP_LOGE(TAG, "Bad array size (arr_size<1).");
        return DHT_BAD_AVG_ARR_SIZE;
    }

    int32_t temperature=0;
    uint32_t humidity=0;

    for (uint8_t i = 0; i < arr_size; ++i)
    {
        temperature+=arr_src[i].temperature;
        humidity+=arr_src[i].humidity;
    }

    dst->temperature = temperature / arr_size;
    dst->humidity = humidity / arr_size;

    return DHT_OK;
}