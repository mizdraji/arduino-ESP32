// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "esp32-hal-bt.h"

#ifdef CONFIG_BT_ENABLED

bool btInUse(){ return true; }

#include "esp_bt.h"

#ifndef BT_MODE
#ifdef CONFIG_BTDM_CONTROLLER_MODE_BTDM
#define BT_MODE ESP_BT_MODE_BTDM
#elif defined(CONFIG_BTDM_CONTROLLER_MODE_BR_EDR_ONLY)
#define BT_MODE ESP_BT_MODE_CLASSIC_BT
#else
#define BT_MODE ESP_BT_MODE_BLE
#endif
#endif

bool btStarted(){
    return (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED);
}

bool btStart(){
    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
#if BT_MODE == ESP_BT_MODE_CLASSIC_BT
    // esp_bt_controller_enable(MODE) This mode must be equal as the mode in “cfg” of esp_bt_controller_init().
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
#elif BT_MODE == ESP_BT_MODE_BLE
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
#endif
    cfg.mode=BT_MODE;

    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED){
        return true;
    }
    esp_err_t ret;
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE){
        if((ret = esp_bt_controller_init(&cfg)) != ESP_OK) {
            log_e("initialize controller failed: %s", esp_err_to_name(ret));
            return false;
        }
        while(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE){}
    }
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED){
        if((ret = esp_bt_controller_enable(BT_MODE)) != ESP_OK) {
            log_e("BT Enable mode=%d failed %s", BT_MODE, esp_err_to_name(ret));
            return false;
        }
    }
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED){
        return true;
    }
    log_e("BT Start failed");
    return false;
}

bool btStop(){
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE){
        return true;
    }
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED){
        if (esp_bt_controller_disable()) {
            log_e("BT Disable failed");
            return false;
        }
        while(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED);
    }
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED){
        if (esp_bt_controller_deinit()) {
			log_e("BT deint failed");
			return false;
		}
		vTaskDelay(1);
		if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_IDLE) {			
			return false;		
		}
        return true;
    }
    log_e("BT Stop failed");
    return false;
}

#else // CONFIG_BT_ENABLED
bool btStarted()
{
    return false;
}

bool btStart()
{
    return false;
}

bool btStop()
{
    return false;
}

#endif // CONFIG_BT_ENABLED

