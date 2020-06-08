//
// Created by Brazen Paradox on 3/6/20.
//

#ifndef SMART_COTTAGE_ESP8266_COMM_UTILS_H
#define SMART_COTTAGE_ESP8266_COMM_UTILS_H

#endif //SMART_COTTAGE_ESP8266_COMM_UTILS_H

int init_comm_port();
int hardware_op(char*);
int get_func(char* ,char* );
int info_func(char* ,int ,struct sockaddr_in);
int intimate_readiness_to_master();

