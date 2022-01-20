/*
 * Copyright (c) 2021, 2022 by
 * Slawomir Krzykala. All rights reserved.
 */ 
#include "pms.h"
#define COMBINE_UINT8(high, low) ( (((uint16_t)high)<<8) | ((uint16_t)low) )
static const char *TAG = "PMS";

static const uint_least8_t PMS_CMD_REQUEST_READ [7] = {0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71};
static const uint_least8_t PMS_CMD_MODE_ACTIVE [7] = {0x42, 0x4D, 0xE1, 0x00, 0X01, 0x01, 0x71};
static const uint_least8_t PMS_CMD_MODE_PASSIVE [7] = {0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70};
//static const uint_least8_t pms_cmd_sleep_mode [7] = {0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73};
//static const uint_least8_t pms_cmd_wakeup_mode [7] = {0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74};


static const uart_config_t PMS_UART_CONFIG = { //config UART from documentation PMS
    .baud_rate = 9600,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 122,
};

static QueueHandle_t pms_uart_queue;

static pms_error_t pms_uart_init(void);



static pms_error_t pms_uart_init(void)//init uart  to pms
{
    // Config UART
    if(!uart_is_driver_installed(PMS_UART_NUM))
    {   
        if(uart_param_config(PMS_UART_NUM , &PMS_UART_CONFIG) !=0 ||
            uart_set_pin(PMS_UART_NUM , PMS_TX_GPIO, PMS_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) !=0 ||
            uart_driver_install(PMS_UART_NUM , PMS_UART_BUFFER_RX_SIZE, PMS_UART_BUFFER_TX_SIZE, 2, &pms_uart_queue, 0) !=0)
        {
            ESP_LOGE(TAG, "Fail init UART.");
            return PMS_FAIL_INIT_UART;
        }
        
    }
    return PMS_OK;
}


pms_error_t pms_init(const pms_workmode_t workmode)//init gpio pms and uart to pms
{
    //config PMS RESET GPIO
    if (gpio_reset_pin(PMS_RESET_GPIO) !=0 ||
        gpio_set_direction(PMS_RESET_GPIO, GPIO_MODE_OUTPUT) !=0 ||
        gpio_set_level(PMS_RESET_GPIO, 1) !=0)
    {
        ESP_LOGE(TAG, "Fail init GPIO RESET");
        return PMS_FAIL_INIT_GPIO;
    }

    //config PMS SET GPIO
    if (gpio_reset_pin(PMS_SET_GPIO) !=0 ||
        gpio_set_direction(PMS_SET_GPIO, GPIO_MODE_OUTPUT) !=0 ||
        gpio_set_level(PMS_SET_GPIO, 1) !=0)
    {
        ESP_LOGE(TAG, "Fail init GPIO SET");
        return PMS_FAIL_INIT_GPIO;
    }

    pms_error_t result = pms_uart_init();
    if(result != 0)
        return result;
    vTaskDelay(600 / portTICK_RATE_MS);
    return pms_set_workmode(workmode);
}


pms_error_t pms_set_workmode(const pms_workmode_t workmode) //send command set mode active/passive
{
    if (workmode == PMS_WORKMODE_ACTIVE)
    {
        if(sizeof(PMS_CMD_MODE_ACTIVE) != uart_tx_chars(PMS_UART_NUM, (const char*)&PMS_CMD_MODE_ACTIVE, sizeof(PMS_CMD_MODE_ACTIVE))) 
        {
            ESP_LOGE(TAG, "Fail send frame - command set active mode.");
            return PMS_FAIL_SEND_FRAME;
        }
    }
    else if(workmode == PMS_WORKMODE_PASSIVE)
    {
        if(sizeof(PMS_CMD_MODE_PASSIVE) != uart_tx_chars(PMS_UART_NUM, (const char*)&PMS_CMD_MODE_PASSIVE, sizeof(PMS_CMD_MODE_PASSIVE))) 
        {
            ESP_LOGE(TAG, "Fail send frame - command set mode passive.");
            return PMS_FAIL_SEND_FRAME;
        }
    }
    
    uart_flush_input(PMS_UART_NUM);
    return PMS_OK;
}


pms_error_t pms_sleep(void)//send command go sleep 
{
    if(gpio_set_level(PMS_SET_GPIO, 0)!=0)
    {
        ESP_LOGE(TAG, "Fail set to 0 GPIO SET.");
        return PMS_FAIL_SET_LEVEL_GPIO;
    }
    return PMS_OK;
}

pms_error_t pms_wake(void)//send command wake up
{
    if(gpio_set_level(PMS_SET_GPIO, 1)!=0)
    {
        ESP_LOGE(TAG, "Fail set to 1 GPIO SET.");
        return PMS_FAIL_SET_LEVEL_GPIO;
    }
    return PMS_OK;
}


pms_error_t pms_request_read(pms_measurement_t *dst) //send command "get measurement results" and read data -> function pms_read_from_buffer
{
    pms_error_t result = pms_uart_init();
    if(result != 0)
        return result;

    uart_flush(PMS_UART_NUM);
    if(sizeof(PMS_CMD_REQUEST_READ) != uart_tx_chars(PMS_UART_NUM, (const char*)&PMS_CMD_REQUEST_READ, sizeof(PMS_CMD_REQUEST_READ)))
    {
        ESP_LOGE(TAG, "Fail send frame - command read data.");
        return PMS_FAIL_SEND_FRAME;
    } 

    vTaskDelay(600 / portTICK_RATE_MS);
    result = pms_read_from_buffer(dst);
    uart_driver_delete(PMS_UART_NUM);
    return result;
}


pms_error_t pms_read_from_buffer(pms_measurement_t *dst) //read data from UART buffor from PMS
{
    uint8_t *received_data=NULL;
    uint16_t length = 0;

//check length data in bufor uart PMS
    uart_get_buffered_data_len(PMS_UART_NUM, (size_t*)&length);

    if(length < 32) //check if data is minimum frame length (32bytes)
    {
        ESP_LOGE(TAG, "Low state of received data in bufor. (%u bytes)", length);
        return PMS_LOW_DATA_BUFOR;
    }

//alloc mem to read data
    received_data = malloc(length * sizeof(uint8_t));
    if(received_data==NULL){
        ESP_LOGE(TAG, "Fail alloc mem (%u bytes) to data from buffer UART.", length);
        return PMS_FAIL_MEMALLOC;
    }

// read data from bufor uart PMS and flush
    length = uart_read_bytes(PMS_UART_NUM, received_data, length, 100);
    uart_flush(PMS_UART_NUM);

// parse received data
    for(uint16_t i=0; i<length; ++i)
    {
        if(received_data[i] == 0x42 && received_data[i+1] == 0x4D)// looking for start frame bytes
        {
        // check whole frame
            if(i+31 >= length)
            {
                ESP_LOGE(TAG, "Frame is not full in received data.");
                free(received_data);
                return PMS_NOT_FULL_FRAME;
            }

        // verification checksum
            uint16_t checksum=0;
            for(uint8_t j=i; j<(i+30); ++j)
                checksum+=received_data[j];

            if(checksum != ((((uint16_t)received_data[30])<<8) | received_data[31]))
            {
                ESP_LOGE(TAG, "Bad checksum received data.");
                free(received_data);
                return PMS_BAD_CHECKSUM;
            }

        // save received data to struct 
            dst->sm.pm10 = COMBINE_UINT8(received_data[i+4], received_data[i+5]); 
            dst->sm.pm25 = COMBINE_UINT8(received_data[i+6], received_data[i+7]); 
            dst->sm.pm100 = COMBINE_UINT8(received_data[i+8], received_data[i+9]); 

            dst->ae.pm10 = COMBINE_UINT8(received_data[i+10], received_data[i+11]); 
            dst->ae.pm25 = COMBINE_UINT8(received_data[i+12], received_data[i+13]); 
            dst->ae.pm100 = COMBINE_UINT8(received_data[i+14], received_data[i+15]); 

            dst->num.um3 = COMBINE_UINT8(received_data[i+16], received_data[i+17]); 
            dst->num.um5 = COMBINE_UINT8(received_data[i+18], received_data[i+19]); 
            dst->num.um10 = COMBINE_UINT8(received_data[i+20], received_data[i+21]); 
            dst->num.um25 = COMBINE_UINT8(received_data[i+22], received_data[i+23]); 
            dst->num.um50 = COMBINE_UINT8(received_data[i+24], received_data[i+25]);
            dst->num.um100 = COMBINE_UINT8(received_data[i+26], received_data[i+27]); 
            
            //break;
            free(received_data);
            return PMS_OK;
        }
    }

    ESP_LOGE(TAG, "Not find frame in received data.");
    free(received_data);
    return PMS_NOT_FIND_FRAME;
}


pms_error_t pms_calc_avg(const pms_measurement_t *arr_src, pms_measurement_t *dst, uint8_t arr_size) //calc avg
{
    if(arr_size < 1)
    {
        ESP_LOGE(TAG, "Bad array size (arr_size<1).");
        return PMS_BAD_AVG_ARR_SIZE;
    }

    uint32_t sum[12]= {0};

    for (uint8_t i = 0; i < arr_size; ++i)
    {    
        sum[0]+=arr_src[i].sm.pm10;
        sum[1]+=arr_src[i].sm.pm25;
        sum[2]+=arr_src[i].sm.pm100; 

        sum[3]+=arr_src[i].ae.pm10;
        sum[4]+=arr_src[i].ae.pm25;
        sum[5]+=arr_src[i].ae.pm100; 

        sum[6]+=arr_src[i].num.um3;
        sum[7]+=arr_src[i].num.um5;
        sum[8]+=arr_src[i].num.um10; 
        sum[9]+=arr_src[i].num.um25;
        sum[10]+=arr_src[i].num.um50;
        sum[11]+=arr_src[i].num.um100; 
    }

    dst->sm.pm10 = sum[0] / arr_size;
    dst->sm.pm25 = sum[1] / arr_size;
    dst->sm.pm100 = sum[2] / arr_size;

    dst->ae.pm10 = sum[3] / arr_size;
    dst->ae.pm25 = sum[4] / arr_size;
    dst->ae.pm100 = sum[5] / arr_size;

    dst->num.um3 = sum[6] / arr_size;
    dst->num.um5 = sum[7] / arr_size;
    dst->num.um10 = sum[8] / arr_size;
    dst->num.um25 = sum[9] / arr_size;
    dst->num.um50 = sum[10] / arr_size;
    dst->num.um100 = sum[11] / arr_size;

    return PMS_OK;
}
