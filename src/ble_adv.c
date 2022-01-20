/*
 * Copyright (c) 2021, 2022 by
 * Slawomir Krzykala. All rights reserved.
 */ 
#include "ble_adv.h"

static const char *TAG = "BLE_ADV";

static esp_bt_controller_config_t ble_adv_bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

static esp_ble_adv_params_t ble_adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_NONCONN_IND, 
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY
};

//Constant part of  data
static ble_adv_head_t ble_adv_head = {
    .len            =0x02,
    .type           =0x01,
    .flags          =0x06,
    .len_payload    =0x1B, 
    .id             =0x0606
};

static uint8_t **ble_adv_data=NULL; 

static uint8_t ble_adv_payload_num=0;
static uint8_t ble_adv_payload_size=0;

static void ble_adv_data_changer_task(void *parameter);
void __attribute__((weak)) ble_adv_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);



ble_adv_error_t ble_adv_bt_init(void) //init BLE, register callback event function 
{
    if(nvs_flash_init() !=0 )
    {
        ESP_LOGE(TAG, "Fail init nvs flash");
        return BLE_ADV_FAIL_INIT_NVS_FLASH;
    }

    ble_adv_bt_cfg.mode=ESP_BT_MODE_BLE;

    if(esp_bt_controller_init(&ble_adv_bt_cfg)!=0 ||
    esp_bt_controller_enable(ESP_BT_MODE_BLE) !=0)
    {
        ESP_LOGE(TAG, "Fail BT controller");
        return BLE_ADV_FAIL_BT_CONTROLLER;
    }
    
    if(esp_bluedroid_init()!=0 ||
    esp_bluedroid_enable()!=0)
    {
        ESP_LOGE(TAG, "Fail bluedroid");
        return BLE_ADV_FAIL_BLUEDROID; 
    }

    if(esp_ble_gap_register_callback(ble_adv_gap_cb) !=0)
    {
        ESP_LOGE(TAG, "Fail register BLE gap callback");
        return BLE_ADV_FAIL_REG_CALLBACK; 
    }

    return BLE_ADV_OK;
}


ble_adv_error_t ble_adv_data_init(uint8_t payload_num, uint8_t payload_size) //init data (only alloc memory and set head frames,
{                                                                            // dont set payload (data)) ble adv and create new task - cyclic changing adv frame 
    ble_adv_data_deinit();

    if(sizeof(ble_adv_head)+payload_size>ESP_BLE_ADV_DATA_LEN_MAX)
    {
        ESP_LOGE(TAG, "Fail init data, size head+payload = (%u) is to large - max 31bytes.", sizeof(ble_adv_head)+payload_size);
        return BLE_ADV_DATA_TOO_SIZE;
    }
    
    ble_adv_payload_num=payload_num;
    ble_adv_payload_size=payload_size;
    ble_adv_head.len_payload=payload_size+2;// 2=size id from head

    ble_adv_data=malloc(sizeof(uint8_t*) * payload_num);

    for(uint8_t i=0; i<payload_num; ++i)
    {
        ble_adv_data[i]=calloc(payload_size+sizeof(ble_adv_head), 1);
        memcpy(ble_adv_data[i], &ble_adv_head, sizeof(ble_adv_head));
    }

    xTaskCreate(ble_adv_data_changer_task, "adv data changer", 1024, NULL, 1, NULL);

    if(esp_ble_gap_start_advertising(&ble_adv_params)!=0)
    {
        ESP_LOGE(TAG, "Fail fail start advertising");
        return BLE_ADV_FAIL_START_ADV; 
    }
    return BLE_ADV_OK;
}


ble_adv_error_t ble_adv_data_deinit(void) //delete adv frames and free memory
{
    esp_ble_gap_stop_advertising();

    if(ble_adv_data!=NULL)
    {
        for(uint8_t i=0; i<ble_adv_payload_num; ++i)
        {
            free(ble_adv_data[i]);
            ble_adv_data[i]=NULL;
        }
        free(ble_adv_data);
        ble_adv_data=NULL;
    }

    ble_adv_payload_num=0;
    ble_adv_payload_size=0;
    return BLE_ADV_OK;
}


ble_adv_error_t ble_adv_set_data(const uint8_t *payload, uint8_t num) //set/change payload (data) adv frame
{   
    if(num>=ble_adv_payload_num)
    {
        ESP_LOGE(TAG, "Copy payload fail, num (%u) is bad number of data.", num);
        return BLE_ADV_FAIL_SET_DATA;
    }

    memcpy(ble_adv_data[num]+sizeof(ble_adv_head), payload, ble_adv_payload_size);
    return BLE_ADV_OK;
}


void __attribute__((weak)) ble_adv_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) //only inform in log status event BLE 
{
    esp_err_t err;

    switch (event) 
    {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        if ((err = param->adv_data_raw_cmpl.status) != ESP_BT_STATUS_SUCCESS) 
        {
            ESP_LOGE(TAG, "Adv set data raw failed: %s", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGV(TAG, "Adv set data raw successfully");
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) 
        {
            ESP_LOGE(TAG, "Adv start failed: %s", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "Adv start successfully");
        }
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if ((err = param->adv_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(TAG, "Adv stop failed: %s", esp_err_to_name(err));
        }
        else 
        {
            ESP_LOGI(TAG, "Stop adv successfully");
        }
        break;

    default:
        break;
    }
}

static void ble_adv_data_changer_task(void *parameter) //task cyclic changing frames, period 100ms
{
    while(1)
    {
        for(uint8_t i=0; i<ble_adv_payload_num; ++i)
        {
            esp_ble_gap_config_adv_data_raw(ble_adv_data[i], sizeof(ble_adv_head)+ble_adv_payload_size);
            vTaskDelay(100 / portTICK_RATE_MS);
            
        }
    }
}

