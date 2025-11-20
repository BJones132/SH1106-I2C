#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#include <esp_log.h>
#include <esp_err.h>
#include <esp_check.h>

#include "panelOps.h"

#define SCL_PIN 22 //I2C Clock GPIO pin
#define SDA_PIN 21 //I2C Data GPIO pin

#define TAG "SH1106_I2C Driver"

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
    vTaskDelay(1 / portTICK_PERIOD_MS);
    gpio_set_level(SCL_PIN, 0);
    vTaskDelay(1 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "I2C bus claimed!");
}

void releaseBus(){
    //To release our bus, return Clock to input, raising via external pullup and then return Data to input, also raising via external pullup.
    gpio_set_level(SCL_PIN, 1);
    gpio_set_direction(SCL_PIN, GPIO_MODE_INPUT);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    gpio_set_level(SDA_PIN, 1);
    gpio_set_direction(SDA_PIN, GPIO_MODE_INPUT);
}

void sendBit(uint32_t level){
    //While Clock is low, set our Data level.
    //Raise Clock for a time then lower Clock.
    gpio_set_level(SDA_PIN, level);
    gpio_set_level(SCL_PIN, 1);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    gpio_set_level(SCL_PIN, 0);
}

int readBit(){
    //To read, set our Data pin to input, raise Clock and check Data level at midpoint of Clock pulse.
    //Lower Clock and set Data pin to output again and return our read bit.
    gpio_set_direction(SDA_PIN, GPIO_MODE_INPUT);
    gpio_set_level(SCL_PIN, 1);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    int res = gpio_get_level(SDA_PIN);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    gpio_set_level(SCL_PIN, 0);
    gpio_set_direction(SDA_PIN, GPIO_MODE_OUTPUT);
    return res;
}

esp_err_t sendByte(uint8_t byte){
    for (int8_t i = 7; i >= 0; i--) { //Read our byte from MSB first to send to I2C bus
        uint32_t bit = (byte & (1ul << i)) >> i; //Using a bit mask to select Ith bit, AND our byte and return bit to LSB to read 1/0 on Ith bit. (Is Ith bit 1 or 0)
        sendBit(bit);
    }
    uint32_t res = readBit();
    if(res == 0){
        return ESP_OK;
    }

    return ESP_ERR_INVALID_RESPONSE;
}

esp_err_t sendCommand(uint8_t cmd){
    ESP_RETURN_ON_ERROR(sendByte(0x80), //Control byte, 1000 0000, Co bit 1 (1 data byte to follow), Data/-Command bit 0, 6 trailing 0s
    TAG, "command control byte returned NACK");

    ESP_RETURN_ON_ERROR(sendByte(cmd),
    TAG, "command byte returned NACK");

    return ESP_OK;
}

esp_err_t sendData(uint8_t data){
    ESP_RETURN_ON_ERROR(sendByte(0xC0),
    TAG, "control byte returned NACK");

    ESP_RETURN_ON_ERROR(sendByte(data),
    TAG, "could not write data: 0x%x", data);

    return ESP_OK;
}

esp_err_t sendDoubleCommand(uint8_t cmd1, uint8_t cmd2){
    ESP_RETURN_ON_ERROR(sendCommand(cmd1),
    TAG, "could not execute command (1/2): 0x%x", cmd1);

    ESP_RETURN_ON_ERROR(sendCommand(cmd2),
    TAG, "could not execute command (2/2): 0x%x", cmd2);

    return ESP_OK;
}

esp_err_t clearDisplay(){
    ESP_RETURN_ON_ERROR(sendCommand(SH1106_CMD_SET_LOWER_COLUMN_ADDR(0)), //Ensure our column address is set to 0 before clearing display
    TAG, "could not set column lower address");

    ESP_RETURN_ON_ERROR(sendCommand(SH1106_CMD_SET_UPPER_COLUMN_ADDR(0)),
    TAG, "could not set column higher address");

    for (size_t i = 0; i < 8; i++)
    {
        ESP_RETURN_ON_ERROR(sendCommand(SH1106_CMD_SET_PAGE_ADDR(i)), //Set our page (8 vertical bits AKA our current 8 bit row)
        TAG, "could not set page address");

        ESP_RETURN_ON_ERROR(sendCommand(SH1106_CMD_BEGIN_RWM), //Begin our RWM loop
        TAG, "could not begin read-write-modify loop");

        for (size_t i = 0; i < 132; i++) {  //SH1106 is 132*64 on paper
            ESP_RETURN_ON_ERROR(sendData(0x00), //within our page, this will set each row of each column to 0 or black/off
            TAG, "could not clear display at index: %u", i);
        }

        ESP_RETURN_ON_ERROR(sendCommand(SH1106_CMD_END_RWM), //End our RWM loop
        TAG, "could not end read-write-modify loop");
    }

    return ESP_OK;
}

esp_err_t initDisplay(){
    claimBus();

    int slaveAddr = 0x78;
    ESP_RETURN_ON_ERROR(sendByte(slaveAddr),
    TAG, "slave could not be reached at: 0x%x", slaveAddr);

    ESP_RETURN_ON_ERROR(sendDoubleCommand(SH1106_CMD_SET_MULTIPLEX_RATIO, SH1106_CMD_DATA_SET_MULTIPLEX_RATIO_DEFAULT),
    TAG, "could not set multiplex ratio");

    ESP_RETURN_ON_ERROR(sendDoubleCommand(SH1106_CMD_SET_COMPINS, SH1106_CMD_DATA_SET_COMPINS_DEFAULT),
    TAG, "could not set common signals pad config");

    ESP_RETURN_ON_ERROR(sendCommand(SH1106_CMD_SET_DISP_OFF),
    TAG, "could not set display off");

    ESP_RETURN_ON_ERROR(sendDoubleCommand(SH1106_CMD_SET_CHARGE_PUMP, SH1106_CMD_DATA_SET_CHARGE_PUMP_ON),
    TAG, "could not set charge pump on");

    ESP_RETURN_ON_ERROR(sendCommand(SH1106_CMD_SET_MIRROR_X_ON), //My screen is "upside down" so I will mirror X by default
    TAG, "could not set mirror x on");

    ESP_RETURN_ON_ERROR(sendCommand(SH1106_CMD_SET_REVERSE_OFF),
    TAG, "could not set reverse off");

    clearDisplay(); //Clear our display while turned off to avoid garbage bits on screen

    ESP_LOGI(TAG, "Display Initialised!");
    return ESP_OK;
}

int displayStatus = 0;
esp_err_t toggleDisplay(){
    if(displayStatus == 0){
        ESP_RETURN_ON_ERROR(sendCommand(SH1106_CMD_SET_DISP_ON),
        TAG, "could not set display on");
        displayStatus = 1;
        ESP_LOGI(TAG, "Display turning on");
    }else{
        ESP_RETURN_ON_ERROR(sendCommand(SH1106_CMD_SET_DISP_OFF),
        TAG, "could not set display off");
        displayStatus = 0;
        ESP_LOGI(TAG, "Display shutting off");
    }
    return ESP_OK;
}

void deinitDisplay(){
    releaseBus();
    ESP_LOGI(TAG, "I2C bus released!");
}

void app_main(void){
    initGPIO();

    if(initDisplay() != ESP_OK){
        ESP_LOGE(TAG, "Display could not be initalised!");
        deinitDisplay();
        return;
    }
    
    if(toggleDisplay() != ESP_OK){
        ESP_LOGE(TAG, "Display could not be toggled!");
        deinitDisplay();
        return;
    }

    deinitDisplay();
}