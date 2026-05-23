idf_component_register(
    SRCS 
        "main.c"
        "parse_json.c"
        "i_modbus/i_modbus.c"
        "i_server/i_server.c"
        "i_wifi_connect/i_wifi_connect.c"
        "i_mqtt_client/i_mqtt_client.c"
        "i_https_client/i_https_client.c"
        "i_sd_mmc/i_sd_mmc.c"
        "i_i2c/i_i2c.c"
                       
    INCLUDE_DIRS
        "."
        "i_modbus"
        "i_server"
        "i_wifi_connect"
        "i_mqtt_client"
        "i_https_client"
        "i_sd_mmc"
        "i_i2c"

    REQUIRES
        driver
        espressif__mqtt
        esp_wifi
        esp_event
        esp_netif
        esp_http_server
        esp_http_client
        esp-tls
        nvs_flash
        espressif__cjson
        fatfs
        sdmmc
        esp_adc
        esp_driver_gpio
        esp_driver_uart
        esp_driver_i2c
        esp_driver_ledc
        esp_driver_touch_sens
        esp_driver_sdmmc
        vfs
        lwip
        mdns
        espressif__esp-modbus

    EMBED_TXTFILES
        "files/index.html"
        "files/sample.txt"
        "../cert/amazon.cer"  

    EMBED_FILES 
        "files/pinout.png"
    
    )


# FAT read only
fatfs_create_rawflash_image(storage ../html FLASH_IN_PROJECT)
# fatfs_create_rawflash_image(storage ${CMAKE_CURRENT_SOURCE_DIR}/../html FLASH_IN_PROJECT)
# fatfs_create_rawflash_image(storage ../site/dist FLASH_IN_PROJECT)

# FAT write only
# fatfs_create_spiflash_image(storage ../flash-loaded FLASH_IN_PROJECT)

# SPIFFS
# spiffs_create_partition_image(storage ../spiffs_dir FLASH_IN_PROJECT)