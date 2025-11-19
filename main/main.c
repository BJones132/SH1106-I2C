#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#include <esp_log.h>

#include "panelOps.h"

#define SCL_PIN 22
#define SDA_PIN 21

#define TAG "I2C Test"

void initGPIO(){
    //I2C GPIO config.
    //External pullups used, therefore disabled on our device.
    gpio_config_t GPIOConf = {
        .pin_bit_mask = (1ULL << SCL_PIN | 1ULL << SDA_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&GPIOConf);
}

void claimBus(){
    //To claim the bus, set our Data and Clock pins to output
    //Data line drops while Clock is high, then Clock drops.
    //Standard I2C start condition.
    gpio_set_direction(SDA_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(SCL_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(SDA_PIN, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(SCL_PIN, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

void sendBit(uint32_t level){
    //While Clock is low, set our Data level.
    //Raise Clock for a time then lower Clock.
    gpio_set_level(SDA_PIN, level);
    gpio_set_level(SCL_PIN, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(SCL_PIN, 0);
}

int readBit(){
    //To read, set our Data pin to input, raise Clock and check Data level at midpoint of Clock pulse.
    //Lower Clock and set Data pin to output again and return our read bit.
    gpio_set_direction(SDA_PIN, GPIO_MODE_INPUT);
    gpio_set_level(SCL_PIN, 1);
    vTaskDelay(5 / portTICK_PERIOD_MS);
    int res = gpio_get_level(SDA_PIN);
    vTaskDelay(5 / portTICK_PERIOD_MS);
    gpio_set_level(SCL_PIN, 0);
    gpio_set_direction(SDA_PIN, GPIO_MODE_OUTPUT);
    return res;
}

uint32_t sendByte(uint8_t byte){
    for (int8_t i = 7; i >= 0; i--) { //Read our byte from MSB first to send to I2C bus
        uint32_t bit = (byte & (1ul << i)) >> i; //Using a bit mask to select Ith bit, AND our byte and return bit to LSB to read 1/0 on Ith bit. (Is Ith bit 1 or 0)
        sendBit(bit);
    }
    uint32_t res = readBit();
    return res;
}

uint32_t sendCommand(uint8_t cmd){
    if(sendByte(0x80) == 0){ //Control byte, 1000 0000, Co bit 1 (1 data byte to follow), Data/-Command bit 0, 6 trailing 0s
        if(sendByte(cmd) == 0){
            return 0;
        }
    }
    ESP_LOGE(TAG, "ERROR: NACK Received!");
    return 1;
}

void releaseBus(){
    //To release our bus, return Clock to input, raising via external pullup and then return Data to input, also raising via external pullup.
    gpio_set_direction(SCL_PIN, GPIO_MODE_INPUT);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_direction(SDA_PIN, GPIO_MODE_INPUT);
}

void initDisplay(){
    claimBus();
    if(sendByte(0x78) == 0) //Current slave address in 8bits, slave addresses are 7bits default followed by R/!W bit. (In this case, 0x3C is our 7bit address, appended with 0 bit leads us to 0x78)
        if(sendCommand(SH1106_CMD_SET_MULTIPLEX_RATIO) == 0) // Multiplex ratio double byte command (1/2)
            if(sendCommand(SH1106_DATA_SET_MULTIPLEX_RATIO_DEFAULT) == 0) // Multiplex ratio double byte command (2/2)
                if(sendCommand(SH1106_CMD_SET_COMPINS) == 0) //Common signals pad config double byte command (1/2)
                    if(sendCommand(SH1106_DATA_SET_COMPINS_DEFAULT) == 0) //Common signals pad config double byte command (2/2)
                        if(sendCommand(SH1106_CMD_SET_DISP_OFF) == 0) //Display off
                            if(sendCommand(SH1106_CMD_SET_CHARGE_PUMP) == 0) //Charge pump control double byte(1/2)
                                if(sendCommand(SH1106_DATA_SET_CHARGE_PUMP_ON) == 0) //Charge pump control double byte(2/2)
                                    if(sendCommand(SH1106_CMD_SET_REVERSE_OFF) == 0) //Reverse display off
                                            ESP_LOGI(TAG, "Display initialised!");
}

void toggleDisplay(int toggle){
    if(toggle == 1){
        sendCommand(SH1106_CMD_SET_DISP_ON);
        ESP_LOGI(TAG, "Display turning on");
    }else{
        sendCommand(SH1106_CMD_SET_DISP_OFF);
        ESP_LOGI(TAG, "Display shutting off");
    }
}

void deinitDisplay(){
    releaseBus();
}

void app_main(void){
    initGPIO();
    initDisplay();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    toggleDisplay(1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    toggleDisplay(0);
    deinitDisplay();
}