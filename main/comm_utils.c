//
// Created by Brazen Paradox on 3/6/20.
//

/*
 * This file holds the code for all sorts of communication code to and from controllers and commanders
 *
 * */

#include<sys/socket.h>
#include<netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "freertos/task.h"

#include "comm_utils.h"
#include "general_utils.h"
#include "devices_utils.h"
#include <unistd.h>

uint16_t comm_port = 10000;

int init_comm_port(){

    printf("\nInitializing comm port\n");
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(sock_fd < 0 ){
        printf("Error occured while trying to create a socket");
        return -1;
    }

    struct sockaddr_in serv_addr, client_addr;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(comm_port);
    serv_addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    bind(sock_fd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr));
    listen(sock_fd, 5);

    uint clilen = sizeof(client_addr);

    printf("\nCOMM port initialized\n");

    while(1) {


        printf("\nListening for incomming connection\n");
        //In lwip_accept the last argument seems to be strictly unsigned value whereas in socket accept of
        //desktop environment, signed integer was expected
        int conn_fd = accept(sock_fd, (struct sockaddr *) &client_addr, &clilen);

        if (conn_fd < 0) {
            printf("Connection failed");
            return -1;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("\nConnection established with host : %s and port %d", client_ip, client_addr.sin_port);

        char recieved_data[256] = {0};
        char reply_message[256] = {0};
        recv(conn_fd, recieved_data, 64, 0);
        //printf("\nRecieved data is %s", recieved_data);

        char method[10];
        int i = 0;
        for (; recieved_data[i] != '\n'; i++) {
            method[i] = recieved_data[i];
        }
        method[i] = '\0';
        printf("method is %s", method);

        if (!strcmp(method, "HARDWARE_OP")) {
            int hardware_op_response = hardware_op(recieved_data);
            if(hardware_op_response == 1){
                strcpy(reply_message, "SUCCESS");
            }
            else if(hardware_op_response == -2){
                strcpy(reply_message, "CONTROLLER_NOT_EXIST");
            }
            else{
                strcpy(reply_message, "FAIL");
            }
        } else if (!strcmp(method, "GET")) {
            //char data_to_send[512];
            get_func(recieved_data, &reply_message);
        }
        else if (!strcmp(method, "INFO")){
            info_func(recieved_data, conn_fd, client_addr);
            continue;
        }
        else{
            strcpy(reply_message, "ILLEGAL_METHOD");
        }
        printf("Reply message is %s\n", reply_message);
        send(conn_fd, reply_message, strlen(reply_message), 0);

        close(conn_fd);
    }

}

int info_func(char *data, int sock_fd, struct sockaddr_in client_addr){

    char *temp = strchr(data, '\n') + 1 ;
    char info[16];
    //Now temp holds the received data after the method i.e INFO

    int i = 0, j=0;
    for(;temp[i] != '\n';)
        info[j++] = temp[i++];
    info[j++] = '\0';

    if(!strcmp(info,"READY")){

        //Send congruence message
        char reply_message[] = "ROGER";
        send(sock_fd, reply_message, strlen(reply_message), 0);

        close(sock_fd);

        //fetch the controller data from slave
        struct sockaddr_in slave_sock_addr;
        slave_sock_addr.sin_family = AF_INET;
        slave_sock_addr.sin_port = htons(comm_port);

        //get client ip and set it in struct
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        slave_sock_addr.sin_addr.s_addr = inet_addr(client_ip);

        int slave_server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(slave_server_sock_fd < 0){
            printf("\nError in socket creation to fetch controller data\n");
            return -1;
        }
        printf("\nSocket created with fd %d\n", slave_server_sock_fd);



        int attempts = 15;
        for(int i= 0 ; i< 6; i++){

            int conn_ret = connect(slave_server_sock_fd,  (struct sockaddr*)(&slave_sock_addr), sizeof(struct sockaddr));
            if(conn_ret >= 0){
                break;
            }
            close(slave_server_sock_fd);
            //Reuse of socket fd whose connection failed will result in unstable behaviour
            slave_server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
            printf("\nTrying to connect with slave attempt %d -- host:%s port: %d and conn ret %d\n", i + 1, client_ip, htons(slave_sock_addr.sin_port), conn_ret);
            if(i == 9){
                return -1;
            }
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }

        char get_controller_data[] = "GET\ncontroller\nEOM\n";
        send(slave_server_sock_fd, get_controller_data, strlen(get_controller_data), 0);

        //printf("\nLet us receive the data\n");

        char serialized_slave_controller_str[512] = {0};
        recv(slave_server_sock_fd, serialized_slave_controller_str, 512, 0);

        close(slave_server_sock_fd);

        cJSON *serialized_slave_controller = cJSON_Parse(serialized_slave_controller_str);
        controller_t slave_controller_struct;
        deserialize_controller(&slave_controller_struct, serialized_slave_controller);
        add_controller_to_list(slave_controller_struct);

        /*char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);*/

        add_controller2ip_map(slave_controller_struct.controller_name, client_ip);

        printf("\nReceived serialized slave controller :%s\n", serialized_slave_controller_str);
    }

    return 1;

}

int get_func(char *data, char* data_to_send){

    char *temp = strchr(data, '\n') + 1 ;
    char data_to_fetch[16];
    //Now temp holds the received data after the method i.e GET

    int i = 0, j=0;
    for(;temp[i] != '\n';)
        data_to_fetch[j++] = temp[i++];
    data_to_fetch[j++] = '\0';

    printf("\nget data: %s\n", data_to_fetch);

    if(!strcmp(data_to_fetch, "controller")){
        extern controller_t controller;

        cJSON *serialized_controller = cJSON_CreateObject();
        serialize_controller(controller, serialized_controller);
        //printf("\nSerialized controller to be sent to requester : %s\n", cJSON_PrintUnformatted(serialized_controller));
        strcpy(data_to_send, cJSON_PrintUnformatted(serialized_controller));
    }

    else if(!strcmp(data_to_fetch, "all")){
        cJSON *all_controllers_serialized = cJSON_CreateArray();
        printf("\nGoing to serialize all controllers\n");
        serialize_all_controllers(all_controllers_serialized);
        printf("All controllers serialized to send %s\n", cJSON_PrintUnformatted(all_controllers_serialized));
        strcpy(data_to_send, cJSON_PrintUnformatted(all_controllers_serialized));
    }

    else{
        strcpy(data_to_send, "ILLEGAL_REQ");
    }

    return 1;

}

int hardware_op(char* data){

    /*
     * data should look like
     *
     * HARDWARE_OP\ncontroller_name\ndevice_name\nvalue\nEOM -- EOM is End Of Message
     *
     * */

    char controller_name[20] = {0};
    char device_name[20] = {0};
    char value_str[9] = {0};

    char *temp = strchr(data, '\n') + 1 ;
    printf("data %s", temp);


    int i = 0, j=0;
    for(;temp[i] != '\n';)
        controller_name[j++] = temp[i++];
    controller_name[j++] = '\0';
    i++;

    for(j=0;temp[i] != '\n';)
        device_name[j++] = temp[i++];
    device_name[j++] = '\0';
    i++;

    for(j=0;temp[i] != '\n';)
        value_str[j++] = temp[i++];
    value_str[j++] = '\0';

    int value = atoi(value_str);

    printf("\ncontroller name:%s -- device name:%s -- value:%d\n", controller_name, device_name, value);

    extern controller_t controller;
    //check if the device name is same as that of the controller
    if(!strcmp(controller_name, controller.controller_name)){
        //if yes, do the operation on the controller
        printf("\nHardware operation is for us\n");

        device_set_value(device_name, value);
        //Write the values in appropriate places
        change_device_value_in_controller_struct(device_name, value);
        change_device_value_in_controller_list(controller_name, device_name, value);

        cJSON* serialized_controller = cJSON_CreateObject();
        serialize_controller(controller, serialized_controller);
        write_to_node_json_file(serialized_controller);

    }
    else {
        //else, send the HARDWARE_OP message to the respective controller after looking up the IP map
        extern ip_controller_name_struct controller_name2ip_map[MAX_CONTROLLERS];
        int ip_index = get_index_from_ip_map(controller_name);
        if(ip_index < 0){
            printf("\nController with name %s is not registered\n", controller_name);
            return -2;
        }

        printf("\nSlave's Ip index is %d\n", ip_index);

        printf("\nHardware operation is for slave -- %s with IP %s\n", controller_name2ip_map[ip_index].controller_name,controller_name2ip_map[ip_index].ip_addr);

        int slave_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(slave_sock_fd < 0 ){
            printf("\nError in socket creation\n");
            return -1;
        }

        struct sockaddr_in slave_sock_addr = {
                .sin_family = AF_INET,
                .sin_port = htons(comm_port)
        };

        slave_sock_addr.sin_addr.s_addr = inet_addr(controller_name2ip_map[ip_index].ip_addr);
        if(connect(slave_sock_fd, (struct sockaddr*)&slave_sock_addr, sizeof(struct sockaddr)) < 0){
            printf("\nCould not connect with IP %s\n", controller_name2ip_map[ip_index].ip_addr);
            close(slave_sock_fd);
            return -3;
        }
        send(slave_sock_fd, data, strlen(data), 0);

        char response_from_slave[10];
        recv(slave_sock_fd, response_from_slave, 10, 0);
        printf("\n data received from slave is %s", response_from_slave);

        if(!strcmp(response_from_slave, "SUCCESS")){
            change_device_value_in_controller_list(controller_name, device_name, value);
        }

        close(slave_sock_fd);
    }
    return 1;


}



//slave operations

// intimate master of readiness

int intimate_readiness_to_master(){

    int master_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in master_sock_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(comm_port),
    };

    master_sock_addr.sin_addr.s_addr = inet_addr("192.168.4.1");


    char readiness_message[] = "INFO\nREADY\nEOM\n";
    char received_data[16] = {0};

    connect(master_sock_fd,  (struct sockaddr*)(&master_sock_addr), sizeof(struct sockaddr));
    send(master_sock_fd, readiness_message, strlen(readiness_message), 0);
    recv(master_sock_fd, received_data, 16, 0);

    printf("\nData received from server for readiness intimation is %s", received_data);
    close(master_sock_fd);

    if(!strcmp(received_data, "ROGER")){
        printf("\nReceived congruence from master. All systems go\n");
        init_comm_port();
    }

    return 1;

}


