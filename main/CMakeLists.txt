idf_component_register(
    SRCS "main.c"
         "gpio/led.c"
         "uart/uart.c"
     #     "lora/transceiver.c"
     #     "lora/mqtt_lora.c"
         "mqtt/mqtt_task.c"
         "wifi/wifi_ap.c"
         "wifi/wifi_station.c"
         "tasks/main_tasks.c"
         "wx_sta/wx_sta.c"
         "wx_sta/wx_app.c"
         "wx_sta/mqtt_wx.c"
         "screen/screen.c"
    INCLUDE_DIRS ".")

target_compile_definitions(${COMPONENT_LIB} PRIVATE 
     WX_STATION
     HUAWEI_CLOUD
)