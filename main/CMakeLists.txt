idf_component_register(
    SRCS "main.c"
         "gpio/led.c"
         "uart/uart.c"
         "lora/transceiver.c"
    INCLUDE_DIRS ".")