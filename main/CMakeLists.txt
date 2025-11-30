idf_component_register(
    SRCS "main.c"
         "gpio/led.c"
         "uart/uart.c"
         "lora/transceiver.c"
         "lora/mqtt_lora.c"
         "mqtt/mqtt_task.c"
         "wifi/wifi_ap.c"
         "wifi/wifi_station.c"
         "tasks/main_tasks.c"
    INCLUDE_DIRS ".")