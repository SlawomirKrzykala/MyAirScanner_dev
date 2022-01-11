// Copyright 2021-2022 Slawomir Krzykala
#include "ble_adv.h"

static const char *TAG = "BLE_ADV";

esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_NONCONN_IND, 
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY
};


//Constant part of  data
ble_adv_head_t adv_head = {
    .len            =0x02,
    .type           =0x01,
    .flags          =0x06,
    .len_payload    =0x1B, 
    .id             =0x0606
};

uint8_t **adv_data=NULL; 

uint8_t adv_payload_num=0;
uint8_t adv_payload_size=0;

void adv_data_changer_task(void *parameter);


ble_adv_error_t ble_adv_bt_init(void)
{
    if(nvs_flash_init() !=0 )
    {
        ESP_LOGE(TAG, "Fail init nvs flash");
        return BLE_ADV_FAIL_INIT_NVS_FLASH;
    }

    bt_cfg.mode=ESP_BT_MODE_BLE;

    if(esp_bt_controller_init(&bt_cfg)!=0 ||
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

    if(esp_ble_gap_register_callback(esp_gap_cb) !=0)
    {
        ESP_LOGE(TAG, "Fail register BLE gap callback");
        return BLE_ADV_FAIL_REG_CALLBACK; 
    }

    return BLE_ADV_OK;
}


ble_adv_error_t ble_adv_data_init(uint8_t payload_num, uint8_t payload_size)
{
    ble_adv_data_deinit();

    if(sizeof(adv_head)+payload_size>ESP_BLE_ADV_DATA_LEN_MAX)
    {
        ESP_LOGE(TAG, "Fail init data, size head+payload = (%u) is to large - max 31bytes.", sizeof(adv_head)+payload_size);
        return BLE_ADV_DATA_TOO_SIZE;
    }
    
    adv_payload_num=payload_num;
    adv_payload_size=payload_size;
    adv_head.len_payload=payload_size+2;

    adv_data=malloc(sizeof(uint8_t*) * payload_num);

    for(uint8_t i=0; i<payload_num; ++i)
    {
        adv_data[i]=calloc(payload_size+sizeof(adv_head), 1);
        memcpy(adv_data[i], &adv_head, sizeof(adv_head));
    }

    xTaskCreate(adv_data_changer_task, "adv data changer", 1024, NULL, 1, NULL);

    if(esp_ble_gap_start_advertising(&adv_params)!=0)
    {
        ESP_LOGE(TAG, "Fail fail start advertising");
        return BLE_ADV_FAIL_START_ADV; 
    }
    return BLE_ADV_OK;
}


ble_adv_error_t ble_adv_data_deinit(void)
{
    esp_ble_gap_stop_advertising();

    if(adv_data!=NULL)
    {
        for(uint8_t i=0; i<adv_payload_num; ++i)
        {
            free(adv_data[i]);
            adv_data[i]=NULL;
        }
        free(adv_data);
        adv_data=NULL;
    }

    adv_payload_num=0;
    adv_payload_size=0;
    return BLE_ADV_OK;
}


ble_adv_error_t ble_adv_set_data(const uint8_t *payload, uint8_t num)
{   
    if(num>=adv_payload_num)
    {
        ESP_LOGE(TAG, "Copy payload fail, num (%u) is bad number of data.", num);
        return BLE_ADV_FAIL_SET_DATA;
    }

    memcpy(adv_data[num]+sizeof(adv_head), payload, adv_payload_size);
    return BLE_ADV_OK;
}


void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
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
            //ESP_LOGV(TAG, "Adv set data raw successfully");
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

void adv_data_changer_task(void *parameter)
{
    while(1)
    {
        for(uint8_t i=0; i<adv_payload_num; ++i)
        {
            esp_ble_gap_config_adv_data_raw(adv_data[i], sizeof(adv_head)+adv_payload_size);
            vTaskDelay(100 / portTICK_RATE_MS);
            
        }
    }
}

