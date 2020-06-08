//
// Created by Brazen Paradox on 1/5/20.
//

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "cJSON.h"

#include "cottage_config.h"
#include "wifi_utils.h"
#include "comm_utils.h"
#include "devices_utils.h"
#include "general_utils.h"

char *TAG ="cottage-main";

void init_spiffs();

controller_t all_controllers[MAX_CONTROLLERS];
ip_controller_name_struct controller_name2ip_map[MAX_CONTROLLERS];

controller_t controller;


void app_main(){

    FILE* json_file;

    //Initialize NVS
    //The NVS module allows you to manage non-volatile memory.
    // Using the NVS APIs, you can read and write data to persistent storage.

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //Initialize spiffs, the filesystem for ESP8266
    ESP_LOGI(TAG, "Initializing SPIFFS");

    init_spiffs();


    //Check if node.json exists

    struct stat st;
    if (!stat("/spiffs/node.json", &st) == 0) {
        //create if it does not exist
        // "w" option corresponds to write mode. It creates the file if it does not exist
        ESP_LOGI(TAG, "node.json does not exist. So creating it..");
        json_file = fopen("/spiffs/node.json", "w");

        if (json_file == NULL) {
            ESP_LOGE(TAG, "Failed to open file for writing");
            return;
        }

        cJSON *controller_json = cJSON_CreateObject();
        controller_json = cJSON_Parse(this_controller_json);

        fprintf(json_file, cJSON_PrintUnformatted(controller_json));
        fclose(json_file);
    }

    json_file = fopen("/spiffs/node.json", "r");
    char controller_json_string[512];
    fgets(controller_json_string, sizeof(controller_json_string), json_file);
    fclose(json_file);

    ESP_LOGI(TAG, "The node json is ");
    ESP_LOGI(TAG, controller_json_string);

    cJSON *controller_json = cJSON_Parse(controller_json_string);

    //initialize the controller object
    init_controller_struct(&controller);
    deserialize_controller(&controller, controller_json);

    //initialize all controllers list
    init_all_controllers_list(&all_controllers);
    all_controllers[0] = controller;


    //Initialize the gpio pins and set levels
    init_gpio_pins();
    init_wifi_adapter();
    if(controller.is_master){
        ESP_LOGI(TAG, "I am a master node");

        start_an_access_point(wifi_ssid, wifi_password);
        init_comm_port();
    }
    else{
        ESP_LOGI(TAG, "I am a slave node. So connecting to access point");
        //connect to the access point if slave
        connect_to_access_point(wifi_ssid, wifi_password);

        //For slave comm port is iniitialized in on_ip_reception_callback of wifi utils
        init_comm_port();

    }




}

void init_spiffs(){

    //Initialize spiffs, the filesystem for ESP8266
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

}