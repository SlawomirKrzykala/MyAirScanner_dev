/*
 * Copyright (c) 2021, 2022 by
 * Slawomir Krzykala. All rights reserved.
 */ 
#include <stdio.h>
#include <memory.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_int_wdt.h>
#include <esp_log.h>

#include "sdkconfig.h"
#include "pms.h"
#include "dht.h"
#include "led_rgb.h"
#include "ble_adv.h"

#define ENCODE_MID_BYTE(high4bits, low4bits) ( ((uint8_t)high4bits<<4) | (((uint8_t)(low4bits>>8))&0x0F) )

#define TIME_SLEEP_MS           300000 //real + DELAY_START_PMS
#define DELAY_START_PMS         40000 //wait minimum 30s to stable data from PMS
#define DELAY_MEASUREMENT       2000 // delay between next measurment
#define MAX_NUM_MEASUREMENT     10 // max number measurment to avg
#define MAX_NUM_TRY_MEASUREMENT 60 // max number try measurment (number = sum of sucess measurment and fail measurment)
static const char *TAG = "DHT";

struct __attribute__((__packed__)) PayloadMeasurement {
    // first 24bits: tttt 	ttyy	hhtt	tttt	hhhh	hhhh  <= t=temp | h=hum | y=type bits
    uint8_t     type:2; 
    uint16_t    temperature:12; //range -400 : 1250 => 0  : 2047 => 1sign bit + 11bits
    uint16_t    humidity:10; //range 0 : 1000 => 0 : 1023 =>10bits
    uint8_t     pm[9];  //6x12bit=9bytes => 2values pm on 3bytes
    uint16_t    um[6];  //6x16bit=12bytes
    uint8_t     esp_temperature; 
};//25bytes

void measure_dht(dht_measurement_t *dht_value_1h);
void measure_pms(pms_measurement_t *pms_value_1h);
void make_adv_data(const dht_measurement_t *dht_value_1h, const pms_measurement_t *pms_value_1h, uint8_t esp_temp, uint8_t type);
static inline uint8_t esp_temp_calc_avg(const uint8_t *arr_src, uint8_t arr_size);
uint8_t temprature_sens_read(void);

uint32_t time_sleep_ms = TIME_SLEEP_MS;

void app_main(void)
{
    dht_measurement_t dht_value_1h[10] = {0};
    pms_measurement_t pms_value_1h[10] = {0};
    uint8_t esp_temp_1h[10]={0};
    uint8_t offset_measurement_1h=0;
    uint8_t num_measurement_1h=0;

    led_rgb_init();
    dht_init();
    pms_init(PMS_WORKMODE_PASSIVE);
    ble_adv_bt_init();
    led_rgb_test();
    led_rgb_set(0,0,0);
    ble_adv_data_init(2, sizeof(struct PayloadMeasurement));
    led_rgb_set(0,0,0);

    while(1)
    {
        measure_dht(&(dht_value_1h[offset_measurement_1h]));
        measure_pms(&(pms_value_1h[offset_measurement_1h]));
        esp_temp_1h[offset_measurement_1h]=temprature_sens_read();
        if(num_measurement_1h<10)++num_measurement_1h; //max 10, fix avg if measurements less than 10, after start esp

        dht_calc_avg(dht_value_1h, &(dht_value_1h[(offset_measurement_1h+1)%10]), num_measurement_1h);
        pms_calc_avg(pms_value_1h, &(pms_value_1h[(offset_measurement_1h+1)%10]), num_measurement_1h);
        esp_temp_1h[(offset_measurement_1h+1)%10] = esp_temp_calc_avg(esp_temp_1h, num_measurement_1h);

        make_adv_data(&(dht_value_1h[offset_measurement_1h]), &(pms_value_1h[offset_measurement_1h]), esp_temp_1h[offset_measurement_1h], 0);//type 0 - data live 
        make_adv_data(&(dht_value_1h[(offset_measurement_1h+1)%10]), &(pms_value_1h[(offset_measurement_1h+1)%10]), esp_temp_1h[(offset_measurement_1h+1)%10], 1);//type 1 - data avg 10 measurements

        if(++offset_measurement_1h>9) offset_measurement_1h=0;

        vTaskDelay(time_sleep_ms / portTICK_RATE_MS);
        time_sleep_ms = TIME_SLEEP_MS;
    }
}

void measure_dht(dht_measurement_t *dht_value_1h)
{
    dht_measurement_t dht_value_tmp[MAX_NUM_MEASUREMENT] = {0};
    uint8_t offset_tmp=0;

    for(uint8_t i=0; i<MAX_NUM_TRY_MEASUREMENT; ++i){
        if(dht_read(&(dht_value_tmp[offset_tmp]))==DHT_OK)
        {
            ESP_LOGI(TAG, "Part (%i) of measurment - Hum: %i Temp: %i \n", offset_tmp, dht_value_tmp[offset_tmp].humidity, dht_value_tmp[offset_tmp].temperature);
            if(++offset_tmp>=MAX_NUM_MEASUREMENT)break;
        } 
        vTaskDelay(DELAY_MEASUREMENT / portTICK_RATE_MS);
        time_sleep_ms-=DELAY_MEASUREMENT;
    }
    dht_calc_avg(dht_value_tmp, dht_value_1h, offset_tmp);

    ESP_LOGI(TAG, "End measurment - Hum: %i Tmp: %i \n", dht_value_1h->humidity, dht_value_1h->temperature);
}

void measure_pms(pms_measurement_t *pms_value_1h)
{
    pms_measurement_t pms_value_tmp[MAX_NUM_MEASUREMENT] = {0};
    uint8_t offset_tmp=0;

    pms_wake();
    vTaskDelay(DELAY_START_PMS / portTICK_RATE_MS);//wait minimum 30s to stable data from PMS
    for(uint8_t i=0; i<MAX_NUM_TRY_MEASUREMENT; ++i){
        if(pms_request_read(&(pms_value_tmp[offset_tmp]))==PMS_OK)
        {
            ESP_LOGI(TAG, "Part (%i) measurment - PM 1/2.5/10: %i/%i/%i \n",offset_tmp, pms_value_tmp[offset_tmp].ae.pm10, pms_value_tmp[offset_tmp].ae.pm25, pms_value_tmp[offset_tmp].ae.pm100);
            if(++offset_tmp>=MAX_NUM_MEASUREMENT)break;
        } 
        vTaskDelay(DELAY_MEASUREMENT / portTICK_RATE_MS);
        time_sleep_ms-=DELAY_MEASUREMENT;
    }
    pms_sleep();

    pms_calc_avg(pms_value_tmp, pms_value_1h, offset_tmp);

    //SET COLOR RGB QUALITY AIR
    if(pms_value_1h->ae.pm100 <= 20 && pms_value_1h->ae.pm25 <= 13)         led_rgb_set(0,255,0);     //green
    else if(pms_value_1h->ae.pm100 <= 50 && pms_value_1h->ae.pm25 <= 35)    led_rgb_set(26,255,26);   //light green
    else if(pms_value_1h->ae.pm100 <= 80 && pms_value_1h->ae.pm25 <= 55)    led_rgb_set(255,30,0);    //yeellow
    else if(pms_value_1h->ae.pm100 <= 110 && pms_value_1h->ae.pm25 <= 75)   led_rgb_set(255,10,0);    //orange
    else if(pms_value_1h->ae.pm100 <= 150 && pms_value_1h->ae.pm25 <= 110)  led_rgb_set(255,4,4);     //light red
    else                                                                    led_rgb_set(255,0,0);     //red

    ESP_LOGI(TAG, "End measurment - PM 1/2.5/10: %i/%i/%i \n", pms_value_1h->ae.pm10, pms_value_1h->ae.pm25, pms_value_1h->ae.pm100);
}

void make_adv_data(const dht_measurement_t *dht_value, const pms_measurement_t *pms_value, uint8_t esp_temp, uint8_t type)
{
    uint16_t dht_temp;
    if(dht_value->temperature>=0)
    {
        dht_temp=dht_value->temperature & 0x07FF;//temperature 11bits, signed 12bit =0
    }
    else
    {
        dht_temp=((dht_value->temperature*-1) & 0x07FF) | 0x0800;//temperature 11bits, sign 12bit =1
    }

    const struct PayloadMeasurement payload={
        .type=type,
        .temperature=dht_temp,//temperature and set sign bit on 12bit
        .humidity=dht_value->humidity,


        .pm[0]=(uint8_t)(pms_value->sm.pm10>>4),
        .pm[1]=ENCODE_MID_BYTE(pms_value->sm.pm10, pms_value->sm.pm25),
        .pm[2]=(uint8_t)pms_value->sm.pm25,
        
        .pm[3]=(uint8_t)(pms_value->sm.pm100>>4),
        .pm[4]=ENCODE_MID_BYTE(pms_value->sm.pm100, pms_value->ae.pm10),
        .pm[5]=(uint8_t)pms_value->ae.pm10,

        .pm[6]=(uint8_t)(pms_value->ae.pm25>>4),
        .pm[7]=ENCODE_MID_BYTE(pms_value->ae.pm25, pms_value->ae.pm100),
        .pm[8]=(uint8_t)pms_value->ae.pm100,


        .um[0]=pms_value->num.um3,
        .um[1]=pms_value->num.um5,
        .um[2]=pms_value->num.um10,
        .um[3]=pms_value->num.um25,
        .um[4]=pms_value->num.um50,
        .um[5]=pms_value->num.um100,

        .esp_temperature=esp_temp
    };

    ble_adv_set_data((const uint8_t*)&payload, type);
}

static inline uint8_t esp_temp_calc_avg(const uint8_t *arr_src, uint8_t arr_size)
{
    if(arr_size<1) return 0;

    uint16_t sum=0;

    for(int i=0; i<arr_size; ++i)
    {
        sum+=arr_src[i];
    }

    return sum/arr_size;
}
