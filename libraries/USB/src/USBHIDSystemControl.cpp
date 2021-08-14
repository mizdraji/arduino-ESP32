// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "esp32-hal.h"
#include "esp32-hal-tinyusb.h"
#include "USBHIDSystemControl.h"

#if CFG_TUD_HID

static uint16_t tinyusb_hid_device_descriptor_cb(uint8_t * dst, uint8_t report_id){
    uint8_t report_descriptor[] = {
        TUD_HID_REPORT_DESC_SYSTEM_CONTROL(HID_REPORT_ID(report_id))
    };
    memcpy(dst, report_descriptor, sizeof(report_descriptor));
    return sizeof(report_descriptor);
}

USBHIDSystemControl::USBHIDSystemControl(): hid(){
	static bool initialized = false;
	if(!initialized){
		initialized = true;
		uint8_t report_descriptor[] = {
		    TUD_HID_REPORT_DESC_SYSTEM_CONTROL(HID_REPORT_ID(0))
		};
		hid.addDevice(this, sizeof(report_descriptor), tinyusb_hid_device_descriptor_cb);
	} else {
		isr_log_e("Only one instance of USBHIDSystemControl is allowed!");
		abort();
	}
}

void USBHIDSystemControl::begin(){
    hid.begin();
}

void USBHIDSystemControl::end(){
}

bool USBHIDSystemControl::send(uint8_t value){
    return hid.SendReport(id, &value, 1);
}

size_t USBHIDSystemControl::press(uint8_t k){
    if(k > 3){
        return 0;
    }
    return send(k);
}

size_t USBHIDSystemControl::release(){
    return send(0);
}

#endif /* CFG_TUD_HID */
