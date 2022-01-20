/*
 * Copyright (c) 2021, 2022 by
 * Slawomir Krzykala. All rights reserved.
 */ 
#ifndef BLE_ADV_H_  
#define BLE_ADV_H_

#include "nvs_flash.h"
#include "esp_log.h"
#include <memory.h>
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include <freertos/task.h>


//max data BLE adv len = 31bytes
typedef struct __attribute__((__packed__)) {
    uint8_t     len;
    uint8_t     type;   
    uint8_t     flags;
    //=================================
    uint8_t     len_payload;//27bytes <= id + ble_adv_payload_t
    uint16_t    id;
} ble_adv_head_t;//6bytes

typedef enum {
    BLE_ADV_OK                  = 0,
    BLE_ADV_FAIL_INIT_NVS_FLASH = -1,
    BLE_ADV_FAIL_BT_CONTROLLER  = -2,
    BLE_ADV_FAIL_BLUEDROID      = -3,
    BLE_ADV_FAIL_REG_CALLBACK   = -4,
    BLE_ADV_DATA_TOO_SIZE       = -5,
    BLE_ADV_FAIL_START_ADV      = -6,
    BLE_ADV_FAIL_SET_DATA       = -7

} ble_adv_error_t;


ble_adv_error_t ble_adv_bt_init(void);
ble_adv_error_t ble_adv_data_init(uint8_t payload_num, uint8_t payload_size);
ble_adv_error_t ble_adv_set_data(const uint8_t *payload, uint8_t num);
ble_adv_error_t ble_adv_data_deinit(void);

#endif