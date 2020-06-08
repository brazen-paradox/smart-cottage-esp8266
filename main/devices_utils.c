//
// Created by Brazen Paradox on 1/5/20.
//

#include "devices_utils.h"
#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_log.h"
#include "string.h"
#include "general_utils.h"

char *DEVICE_UTILS_TAG = "DEVICE UTILS TAG";


// initialize all the output pins and Switch levels of pins according to node_json

int init_gpio_pins(){

    extern controller_t controller;

    for(int i =0; i<MAX_DEVICES; i++){

        if(controller.devices[i].pin > 0) {
            //Set the GPIO to output mode
            gpio_set_direction(controller.devices[i].pin, GPIO_MODE_OUTPUT);

            //set the level of gpio pin
            gpio_set_level(controller.devices[i].pin, controller.devices[i].value);
        }
    }

    //May use the return integer to notify errors
    return 1;
}

int device_set_value(char* device_name, int value){

    extern controller_t controller;
    for(int i =0; i<MAX_DEVICES; i++){

        if(!strcmp(controller.devices[i].device_name, device_name)) {
            //set the level of gpio pin
            gpio_set_level(controller.devices[i].pin, value);
        }
    }

    return 1;
}