#ifndef MAIN_H
#define MAIN_H

#pragma once

#include "mongoose.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"

#define FS_ROOT "/spiffs"
#define WIFI_FILE FS_ROOT "/wifi.json"
#define UART_NO 1

void uart_bridge_fn(struct mg_connection *, int, void *, void *);
int uart_read(void *buf, size_t len);
bool wifi_init(const char *ssid, const char *pass);
void app_wifi_init();
bool wifi_init_prov();
bool wifi_prev();
void cli(uint8_t ch);
void cli_init(void);

//GPIO_NUM_0 is used for LED BATERIA
//GPIO_NUM_22 is used for LED COMUNICACION
//GPIO_NUM_2 is used for LED ON
//GPIO_NUM_23 is used for LED FALLA
//GPIO_NUM_21 is used for LED ALARMA
//GPIO_NUM_18 is used for BUTTON
//
//#define CONFIG_ELKRON_EMULATOR 1

#ifdef CONFIG_IDF_TARGET_ESP32
  #define LED_BATERIA GPIO_NUM_0
  #define LED_COMUNICACION GPIO_NUM_22
  #define LED_ON GPIO_NUM_2
  #define LED_FALLA GPIO_NUM_23
  #define LED_ALARMA GPIO_NUM_21
  #define LED_DC GPIO_NUM_19
  #define BUTTON GPIO_NUM_18
  #define PRESSED_BTN 1
#endif

#ifdef CONFIG_IDF_TARGET_ESP32C3
  #define LED_BATERIA GPIO_NUM_8
  #define LED_COMUNICACION GPIO_NUM_8
  #define LED_ON GPIO_NUM_8
  #define LED_FALLA GPIO_NUM_8
  #define LED_ALARMA GPIO_NUM_8
  #define LED_DC GPIO_NUM_8
  #define BUTTON GPIO_NUM_9
  #define PRESSED_BTN 0
#endif

#endif  // MAIN_H
