//
// Created by Brazen Paradox on 1/5/20.
//

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "rom/ets_sys.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "wifi_utils.h"
#include "lwip/ip4_addr.h" //To import IP address related functions and structs
#include "comm_utils.h"

#define AP_MAX_CONN	4

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

char *WIFI_TAG = "smart-cottage-wifi_utils";

int get_length(uint8_t*);

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    /* For accessing reason codes in case of disconnection */
    system_event_info_t *info = &event->event_info;

    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(WIFI_TAG, "Wifi station started");
            scan_for_aps();
            break;

        case SYSTEM_EVENT_SCAN_DONE:
            ESP_LOGI(WIFI_TAG, "Scan Completed Successfully.");
            list_ap();
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            //For slave alone
            ESP_LOGI(WIFI_TAG, "got ip:%s",
                     ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
            //Since the IP received it can be used to start the server
            on_ip_reception_callback();

            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(WIFI_TAG, "station:"MACSTR" join, AID=%d",
                     MAC2STR(event->event_info.sta_connected.mac),
                     event->event_info.sta_connected.aid);
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(WIFI_TAG, "station:"MACSTR" leave, AID=%d",
                     MAC2STR(event->event_info.sta_disconnected.mac),
                     event->event_info.sta_disconnected.aid);
            break;
        case SYSTEM_EVENT_AP_STAIPASSIGNED:
            //for master alone
            ESP_LOGI(WIFI_TAG, "New device connected");
            // IP address assigned to another device that is connected
            ip4_addr_t set_ip_addr = event->event_info.ap_staipassigned.ip;
            ESP_LOGI(WIFI_TAG, "Device connected to smart-cottage via wifi is assigned IP : %s",
                     ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));

            on_ip_set_callback(set_ip_addr);



        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGE(WIFI_TAG, "Disconnect reason : %d", info->disconnected.reason);
            if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
                /*Switch to 802.11 bgn mode */
                esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
            }

            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

void start_an_access_point(uint8_t* access_point_ssid, uint8_t* access_point_password)
{

    ESP_LOGI(WIFI_TAG, "Gonna create an access point with creds SSID=%s, PASSWORD=%s", access_point_ssid, access_point_password);

    //Create a wifi configuration type variable to provide details for it to act as an access point
    wifi_config_t wifi_config = {
            .ap = {
                    .max_connection = AP_MAX_CONN,
                    .authmode = WIFI_AUTH_WPA2_PSK,
            },
    };


    uint8_t ssid_length = get_length(access_point_ssid);
    memcpy(wifi_config.ap.ssid, access_point_ssid, 32);
    memcpy(wifi_config.ap.password, access_point_password, 64);
    // wifi_config.ap.ssid_len should be the one excluding the null termination character I guess.
    wifi_config.ap.ssid_len=ssid_length;

    if (strlen((char*)access_point_password) == 0) {
        //When you do not care about people peeping to your network, do this.
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_TAG, "wifi_init_softap finished.SSID:%s password:%s channel:%d ssid length: %d",
             access_point_ssid, access_point_password, wifi_config.ap.channel, wifi_config.ap.ssid_len);
}

void connect_to_access_point(uint8_t* ssid, uint8_t* password)
{

    ESP_LOGI(WIFI_TAG, "Gonna connect to the access point with creds SSID=%s, PASSWORD=%s", ssid, password);

    //Create a wifi configuration type variable to provide details of the access point to which it has to connect
    wifi_config_t wifi_config = {
            .sta = {},
    };

    /*uint8_t ssid_[] = "cottage-host";
    uint8_t password_[] = "eooooasdff";*/
    memcpy(wifi_config.sta.ssid, ssid, 32);
    memcpy(wifi_config.sta.password, password, 64);

    //strcpy((char*)wifi_config.sta.ssid, (char*)ssid);
    //strcpy((char*)wifi_config.sta.password, (char*)password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_TAG, "wifi_init_sta finished.");
    ESP_LOGI(WIFI_TAG, "connect to ap SSID:%s password:%s channel:%d",
             wifi_config.sta.ssid, wifi_config.sta.password, wifi_config.sta.channel);
}

void scan_for_aps(){

    //wifi_scan_config_t scan_config;
/*    scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    scan_config.scan_time.active.min = 1000; // minimum of 1000 ms to scan each channel
    scan_config.scan_time.active.max = 1500; // maximum of 1500 ms to scan each channel*/

    //scan_config.scan_type = WIFI_SCAN_TYPE_PASSIVE;

    wifi_scan_config_t scan_config = {
            .channel = 0,
            .show_hidden = true,
            .scan_type = WIFI_SCAN_TYPE_ACTIVE,
            .scan_time.active.min = 10,
            .scan_time.active.max = 100
    };

    ESP_LOGI(WIFI_TAG, "Beginning scan!");
    ESP_LOGI(WIFI_TAG, "Scan Type:%d", scan_config.scan_type);

    esp_wifi_scan_start(&scan_config, true);

}

void list_ap(){


    ESP_LOGI(WIFI_TAG, "Going to list the aps");
    uint16_t ap_count = 5; //max number of access points to scan

    if(esp_wifi_scan_get_ap_num(&ap_count) == ESP_OK) {
        ESP_LOGI(WIFI_TAG, "Total number of scanned APs is %d", ap_count);

        wifi_ap_record_t *ap_records = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_count); // Array of wifi_ap_record to hold the access points data

        esp_wifi_scan_get_ap_records(&ap_count, ap_records);

        for (int i=0; i<ap_count; i++){
            ESP_LOGI(WIFI_TAG, "%d.SSID: %s  Channel: %d  authmode: %d country %s", i+1, ap_records[i].ssid, ap_records[i].primary,
                    ap_records[i].authmode, ap_records[i].country.cc);

            uint8_t check[] = "grasshopper";
            if(!memcmp(ap_records[i].ssid, check, sizeof(ap_records[i].ssid))){
                ESP_LOGI(WIFI_TAG, "Yay we have a match %s", ap_records[i].ssid);
            }
        }

    }
    else{
        ESP_LOGI(WIFI_TAG, "An error occured while trying to fetch the list of APs");
    }

}

void init_wifi_adapter(){
    //Pre-requisites -- initialization if you will to get the device working
    wifi_event_group = xEventGroupCreate();

    //Initialize the wifi adapter
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    /*Initialize the wifi device with the macros -- default valuess
     * Ref: https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/network/esp_wifi.html
     * search for WIFI_INIT_CONFIG_DEFAULT
    */

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    //esp_wifi_set_storage(WIFI_STORAGE_RAM);
}

//slave's function
void on_wifi_connect_callback(){

}

//Slave's function
void on_ip_reception_callback(){
    // Bind the received IP to start a webserver
    ESP_LOGI(WIFI_TAG, "Going to intimate readiness to master");

    //Tell master that the webserver is ready
    intimate_readiness_to_master();
    //init_comm_port();


}

//For master
// This function has to be called after IP is assigned to the connected device
void on_ip_set_callback(ip4_addr_t set_ip_addr){

}

void add_ip_to_node_json(char* ip_address, char* node_name){

}

int get_length(uint8_t *str){
    int len=0;
    //printf("\nstring is %s", str);
    while(str[len] != 0){
        //printf("\nchar is %c", str[len]);
        len++;
    }

    //return sizeof(str);
    //Add one for null termination -- may not be required 18 may 2020
    return len;
}