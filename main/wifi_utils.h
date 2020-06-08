//
// Created by Brazen Paradox on 1/5/20.
//

#ifndef SMART_COTTAGE_ESP8266_WIFI_UTILS_H
#define SMART_COTTAGE_ESP8266_WIFI_UTILS_H

#endif //SMART_COTTAGE_ESP8266_WIFI_UTILS_H

void connect_to_access_point(uint8_t* ssid, uint8_t* password);
void start_an_access_point(uint8_t* access_point_ssid, uint8_t* access_point_password);
void init_wifi_adapter();
void on_ip_reception_callback();
void on_ip_set_callback();
void scan_for_aps();
void list_ap();