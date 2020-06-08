cmake_minimum_required(VERSION 3.12)
project(smart_cottage_esp8266 C)

set(CMAKE_C_STANDARD 11)

#[[include_directories(.)
include_directories(main)]]

add_executable(smart_cottage_esp8266
    main/app_main.c
    main/comm_utils.c
    main/comm_utils.h
        main/cottage_config.h
    main/devices_utils.c
    main/devices_utils.h
        main/wifi_utils.c
    main/wifi_utils.h
    cottage_master_config.h
    guest_house_config.h main/general_utils.c main/general_utils.h)
