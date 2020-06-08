//
// Created by Brazen Paradox on 6/6/20.
//

#include "general_utils.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>


int serialize_device(device_t device, cJSON *serialized_device){

    cJSON_AddNumberToObject(serialized_device, "pin", device.pin);
    cJSON_AddNumberToObject(serialized_device, "value", device.value);
    cJSON_AddStringToObject(serialized_device, "name", device.device_name);

    //printf("\n%s", cJSON_PrintUnformatted(serialized_device));
    return 1;
}

int serialize_controller(controller_t controller, cJSON *serialized_controller){

    cJSON_AddStringToObject(serialized_controller, "name", controller.controller_name);
    cJSON_AddNumberToObject(serialized_controller, "is_master", controller.is_master);

    cJSON *devices_json_array = cJSON_CreateArray();

    for(int i=0; i<MAX_DEVICES; i++){
        //printf("\nLet us serialize %d %d", i, controller.devices[i].pin);
        if(controller.devices[i].pin <= 0){
            break;
        }
        cJSON *serialized_device = cJSON_CreateObject();
        serialize_device(controller.devices[i], serialized_device);
        cJSON_AddItemToArray(devices_json_array, serialized_device);
    }

    //printf(cJSON_PrintUnformatted(devices_json_array));
    cJSON_AddItemToObject(serialized_controller, "devices", devices_json_array);
    printf("\n\nSerialize controller function result : %s\n", cJSON_PrintUnformatted(serialized_controller));

    return 1;
}

int serialize_all_controllers(cJSON* serialized_all_controllers){
    extern controller_t all_controllers[MAX_CONTROLLERS];

    for(int i = 0; i<MAX_CONTROLLERS; i++){
        if(!strcmp(all_controllers[i].controller_name, "")){
            break;
        }

        printf("\n present controller name : %s", all_controllers[i].controller_name);

        cJSON *controller_json = cJSON_CreateObject();

        serialize_controller(all_controllers[i], controller_json);
        printf("\n Controller serialized now :%s",  cJSON_PrintUnformatted(controller_json));

        cJSON_AddItemToArray(serialized_all_controllers, controller_json);
        //cJSON_Delete(controller_json);
    }
    printf("\nSerialized all controllers : %s\n", cJSON_PrintUnformatted(serialized_all_controllers));

    return 1;

}

int deserialize_device(device_t *device, cJSON *serialized_device){
    strcpy(device->device_name, cJSON_GetObjectItem(serialized_device, "name")->valuestring);
    device->pin =  cJSON_GetObjectItem(serialized_device, "pin")->valueint;
    device->value = cJSON_GetObjectItem(serialized_device, "value")->valueint;

    return 1;
}

int deserialize_controller(controller_t *controller, cJSON *serialized_controller){

    //printf("\n\nsupposedly %d", cJSON_GetObjectItem(serialized_controller, "is_master")->valueint);
    strcpy(controller->controller_name, cJSON_GetObjectItem(serialized_controller, "name")->valuestring);
    controller->is_master = cJSON_GetObjectItem(serialized_controller, "is_master")->valueint;

    //add devices
    cJSON *devices_json = cJSON_GetObjectItem(serialized_controller, "devices");
    cJSON *serialized_device = devices_json->child;


    for(int index = 0; serialized_device; index++){

        device_t device;
        deserialize_device(&device, serialized_device);
        controller->devices[index] = device;
        printf("\n\nDeserialized device :->  name:%s , pin: %d", controller->devices[index].device_name, device.pin);
        serialized_device = serialized_device->next;
    }

    //printf("\n\nDeserialized controller :\nname: %s\nis master:%d", controller.controller_name, controller.is_master);
    return 1;

}

int init_controller_struct(controller_t *controller){

    for(int i = 0; i<MAX_DEVICES; i++){
        controller->devices[i].pin = -1;
    }

    return 1;
}

int init_all_controllers_list(controller_t *controller_list){

    for(int i = 0; i<MAX_CONTROLLERS; i++){
        strcpy(controller_list[i].controller_name, "");
        printf("\n%s", controller_list[0].controller_name);
    }
    printf("\n%s", controller_list[0].controller_name);
    printf("\n%s\n", controller_list[1].controller_name);

    return 1;

}

int add_controller_to_list(controller_t controller){
    extern controller_t all_controllers[MAX_CONTROLLERS];
    int i;
    for(i=0; i < MAX_CONTROLLERS; i++) {
        if(!strcmp(all_controllers[i].controller_name, "")) {
            printf("\nNext free index in controller is %d\n", i);
            all_controllers[i] = controller;
            return 1;
        }
        else if(!strcmp(all_controllers[i].controller_name, controller.controller_name)){
            printf("\nController with the same name exists in index %d, so overwriting it \n", i);
            all_controllers[i] = controller;

            //two means overwritten existing controller
            return 2;
        }
    }

    //-1 means that there is no space for a new controller to be added
    return -1;


}

int add_controller2ip_map(char* controller_name, char* ip_addr){
    extern ip_controller_name_struct controller_name2ip_map[MAX_CONTROLLERS];
    int i;
    for(i=0; i < MAX_CONTROLLERS; i++) {
        if(!strcmp(controller_name2ip_map[i].controller_name, "")){
            printf("\nNext free index in ip map is %d\n", i);
            strcpy(controller_name2ip_map[i].ip_addr, ip_addr);
            strcpy(controller_name2ip_map[i].controller_name, controller_name);
            return 1;
        }
        else if (!strcmp(controller_name2ip_map[i].controller_name, controller_name)){
            printf("\ncontroller with same name already exists in ip map in index %d, so overwriting it\n", i);
            strcpy(controller_name2ip_map[i].ip_addr, ip_addr);
            return 1;
        }

    }



    return -1;
}

int get_index_from_ip_map(char* controller_name){
    extern ip_controller_name_struct controller_name2ip_map[MAX_CONTROLLERS];
    int index = -1;
    for(int i=0; i < MAX_CONTROLLERS; i++) {
        if (!strcmp(controller_name2ip_map[i].controller_name, controller_name)){
            index = i;
            break;
        }
    }
    return index;
}

int change_device_value_in_controller_struct(char* device_name, int value){
    extern controller_t controller;
    for(int i=0; i < MAX_DEVICES; i++){
        if(controller.devices[i].pin > 0 && !strcmp(controller.devices[i].device_name, device_name)){
            controller.devices[i].value = value;
        }
    }
    return 1;
}

int change_device_value_in_controller_list(char* controller_name, char* device_name, int value){
    extern controller_t all_controllers[MAX_CONTROLLERS];
    for(int j =0; j < MAX_CONTROLLERS; j++){
        printf("controller name we are looking for is %s and device %s with value %d", controller_name, device_name, value);
        if(!strcmp(all_controllers[j].controller_name, controller_name)) {
            printf("\ncontroller was in index %d\n", j);
            for (int i = 0; i < MAX_DEVICES; i++) {
                printf("\n The present device is %s and the device we are looking for is %s", all_controllers[j].devices[i].device_name, device_name);
                if (all_controllers[j].devices[i].pin > 0 && !strcmp(device_name, all_controllers[j].devices[i].device_name)) {
                    printf("\nDevice was in index %d\n", i);
                    all_controllers[j].devices[i].value = value;
                    return 1;
                }
            }
            break;
        }
    }
    return -1;
}



// ROM Loading and writing

int write_to_node_json_file(cJSON* node_json){

    FILE* json_file = fopen("/spiffs/node.json", "w");
    fprintf(json_file, cJSON_PrintUnformatted(node_json));
    fclose(json_file);

    //May use the return integer to notify errors
    return 0;
}