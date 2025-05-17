// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved

#include "main.h"
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_softap.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"


const char *s_listening_url = "http://0.0.0.0:80";
#define TAG "main"

char *config_read(void)
{
  return mg_file_read(&mg_fs_posix, FS_ROOT "/config.json", NULL);
}

void config_write(struct mg_str config)
{
  mg_file_write(&mg_fs_posix, FS_ROOT "/config.json", config.ptr, config.len);
}

void btn_task(void *arg)
{
  ESP_LOGI(TAG, "Init button task...");
  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(500));
    if (gpio_get_level(GPIO_NUM_18) == 1)
    {
      ESP_LOGI(TAG, "Button pressed, monitoring duration...");

      int seconds_held = 0;
      bool ten_sec_action_done = false;

      while (gpio_get_level(GPIO_NUM_18) == 1 && seconds_held < 12)
      {
        vTaskDelay(pdMS_TO_TICKS(1000));
        seconds_held++;
        ESP_LOGI(TAG, "Held for %d seconds", seconds_held);

        if (seconds_held == 10 && !ten_sec_action_done)
        {
          ESP_LOGI(TAG, "Button held for 10 seconds. Entering provisioning...");
          wifi_prov_mgr_reset_provisioning();
      esp_restart();

          ten_sec_action_done = true;
        }
      }

      if (seconds_held >= 3 && seconds_held < 10)
      {
        ESP_LOGI(TAG, "Button held for 3 seconds. Restarting...");
        esp_restart();
      }
      else if (seconds_held < 3)
      {
        ESP_LOGI(TAG, "Button released before 3 seconds.");
      }
    }
  }
}
/*void btn_task(void *arg)
{
  ESP_LOGI(TAG, "Init button task...");
  while (1)
  {
    //ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // Wait for button press

    // Debounce delay
    vTaskDelay(pdMS_TO_TICKS(500));
    if (gpio_get_level(GPIO_NUM_18) == 1)
    {
      ESP_LOGI(TAG, "Button pressed, monitoring duration...");

      int seconds_held = 0;
      bool five_sec_action_done = false;

      while (gpio_get_level(GPIO_NUM_18) == 1 && seconds_held < 11)
      {
        vTaskDelay(pdMS_TO_TICKS(1000));
        seconds_held++;
        ESP_LOGI(TAG, "Held for %d seconds", seconds_held);

        if (seconds_held == 5 && !five_sec_action_done)
        {
          ESP_LOGI(TAG, "Button held for 5 seconds. Performing provisioning...");

          wifi_prov_mgr_reset_provisioning();
        app_wifi_init();

          five_sec_action_done = true;
        }

        if (seconds_held == 10)
        {
          ESP_LOGI(TAG, "Button held for 10 seconds. Restarting...");
          esp_restart();
        }
      }

      if (seconds_held < 5)
      {
        ESP_LOGI(TAG, "Button released before 5 seconds.");
      }
      else if (seconds_held < 10)
      {
        ESP_LOGI(TAG, "Button released after 5 but before 10 seconds.");
      }
    }
  }
}*/

#include "esp_log.h"
void app_main(void)
{

  // Mount filesystem
  esp_vfs_spiffs_conf_t conf = {
      .base_path = FS_ROOT, .max_files = 20, .format_if_mount_failed = true, .partition_label = "storage"};
  int res = esp_vfs_spiffs_register(&conf);
  //  if (res != ESP_OK) {
  //    res = esp_spiffs_format(conf.partition_label);
  //  }

  MG_INFO(("FS at %s initialised, status: %d, partition label: %s", conf.base_path, res, conf.partition_label));

  // Try to connect to wifi by using saved WiFi credentials
  char *json = mg_file_read(&mg_fs_posix, WIFI_FILE, NULL);

  // LED BATERIA
  gpio_set_direction(GPIO_NUM_0, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_0, 0);

  // LED COMUNICACION
  gpio_set_direction(GPIO_NUM_22, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_22, 0);

  // LED ON
  gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_2, 0);

  // LED FALLA
  gpio_set_direction(GPIO_NUM_23, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_23, 0);

  // LED ALARMA
  gpio_set_direction(GPIO_NUM_21, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_21, 0);

  gpio_set_direction(GPIO_NUM_18, GPIO_MODE_INPUT);

  gpio_set_level(GPIO_NUM_2, 1);

//  gpio_install_isr_service(0);
//  gpio_isr_handler_add(GPIO_NUM_18, button_isr_handler, NULL);

  xTaskCreate(btn_task, "btn_task", 2048, NULL, 10, NULL);

  app_wifi_init();

  struct mg_mgr mgr;
  mg_mgr_init(&mgr);
  mg_log_set(MG_LL_DEBUG); // Set log level
  MG_INFO(("Mongoose v%s on %s", MG_VERSION, s_listening_url));
  mg_http_listen(&mgr, s_listening_url, uart_bridge_fn, &mgr);
  //  uint64_t last_ms = 0;

  for (;;)
  {
    gpio_set_level(GPIO_NUM_2, 0);
    mg_mgr_poll(&mgr, 150);
    gpio_set_level(GPIO_NUM_2, 1);
    mg_mgr_poll(&mgr, 150);
  }; // Infinite event loop
}
