//
// Created by Brazen Paradox on 6/6/20.
//

#ifndef SMART_COTTAGE_ESP8266_GENERAL_UTILS_H
#define SMART_COTTAGE_ESP8266_GENERAL_UTILS_H

#endif //SMART_COTTAGE_ESP8266_GENERAL_UTILS_H

#ifndef MAX_CONTROLLERS
#define MAX_CONTROLLERS 5

#endif

#ifndef MAX_DEVICES
#define MAX_DEVICES 7

#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "cJSON.h"

typedef struct {
    int pin;
    char device_name[15];
    uint32_t value;
} device_t;

typedef struct {
    char controller_name[15];
    int is_master;
    device_t devices[MAX_DEVICES];
} controller_t;

typedef struct{
    char controller_name[15];
    char ip_addr[16];
} ip_controller_name_struct;

int serialize_device(device_t, cJSON*);
int serialize_controller(controller_t, cJSON*);
int deserialize_controller(controller_t*, cJSON*);
int deserialize_device(device_t*, cJSON*);
int init_controller_struct(controller_t*);
int add_controller_to_list(controller_t);
int add_controller2ip_map(char*, char*);
int init_all_controllers_list(controller_t*);
int get_index_from_ip_map(char*);
int serialize_all_controllers(cJSON*);
int change_device_value_in_controller_list(char*, char* , int);
int change_device_value_in_controller_struct(char*, int);
int write_to_node_json_file(cJSON* );
