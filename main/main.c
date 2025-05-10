// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved

#include "main.h"
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_softap.h>

const char *s_listening_url = "http://0.0.0.0:80";

char *config_read(void) {
  return mg_file_read(&mg_fs_posix, FS_ROOT "/config.json", NULL);
}

void config_write(struct mg_str config) {
  mg_file_write(&mg_fs_posix, FS_ROOT "/config.json", config.ptr, config.len);
}





void app_main(void) {

  // Mount filesystem
  esp_vfs_spiffs_conf_t conf = {
      .base_path = FS_ROOT, .max_files = 20, .format_if_mount_failed = true, .partition_label = "storage"};
  int res = esp_vfs_spiffs_register(&conf);
//  if (res != ESP_OK) {
//    res = esp_spiffs_format(conf.partition_label);
//  }

  MG_INFO(("FS at %s initialised, status: %d, partition label: %s", conf.base_path, res,conf.partition_label));

  // Try to connect to wifi by using saved WiFi credentials
  char *json = mg_file_read(&mg_fs_posix, WIFI_FILE, NULL);



//LED BATERIA
gpio_set_direction(GPIO_NUM_0,GPIO_MODE_OUTPUT);
gpio_set_level(GPIO_NUM_0, 0);



  gpio_set_direction(GPIO_NUM_18,GPIO_MODE_INPUT);

  //LED COMUNICACION
  gpio_set_direction(GPIO_NUM_22,GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_22, 0);

  //LED ON
  gpio_set_direction(GPIO_NUM_2,GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_2, 0);

  //LED FALLA
  gpio_set_direction(GPIO_NUM_23,GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_23, 0);

  //LED ALARMA
  gpio_set_direction(GPIO_NUM_21,GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_21, 0);


  app_wifi_init();



  if (json != NULL && gpio_get_level(GPIO_NUM_18) == 0)
  {
    char *ssid = mg_json_get_str(mg_str(json), "$.ssid");
    char *pass = mg_json_get_str(mg_str(json), "$.pass");
    wifi_init(ssid, pass);
    free(ssid);
    free(pass);
    free(json);
  }
  else
  {
    // ACA
    MG_INFO(("WiFi is not configured, create WIFI AP Prov"));

    wifi_init_prov();

    // If WiFi is not configured, run CLI until configured
    MG_INFO(("WiFi is not configured, running CLI. Press enter"));
    cli_init();



    for (;;)
    {
static int current_led = 0;
static uint32_t last_toggle_time = 0;
static bool direction_forward = true; // Control the direction of the effect
uint32_t current_time = esp_timer_get_time() / 1000;

if (current_time - last_toggle_time > 50) { // Change LED every 200ms
    last_toggle_time = current_time;

    // Turn off all LEDs before changing the current one
    gpio_set_level(GPIO_NUM_2, 0);  // LED ON
    gpio_set_level(GPIO_NUM_19, 0); // LED DC
    gpio_set_level(GPIO_NUM_0, 0);  // LED BATERIA
    gpio_set_level(GPIO_NUM_21, 0); // LED ALARMA
    gpio_set_level(GPIO_NUM_22, 0); // LED COMUNICACION
    gpio_set_level(GPIO_NUM_23, 0); // LED FALLA

    // Turn on the current LED
    switch (current_led) {
        case 0: gpio_set_level(GPIO_NUM_2, 1); break;  // LED ON
        case 1: gpio_set_level(GPIO_NUM_19, 1); break; // LED DC
        case 2: gpio_set_level(GPIO_NUM_0, 1); break;  // LED BATERIA
        case 3: gpio_set_level(GPIO_NUM_21, 1); break; // LED ALARMA
        case 4: gpio_set_level(GPIO_NUM_22, 1); break; // LED COMUNICACION
        case 5: gpio_set_level(GPIO_NUM_23, 1); break; // LED FALLA
    }

    // Update the current LED index based on the direction
    if (direction_forward) {
        current_led++;
        if (current_led > 5) {
            current_led = 4; // Reverse direction at the last LED
            direction_forward = false;
        }
    } else {
        current_led--;
        if (current_led < 0) {
            current_led = 1; // Reverse direction at the first LED
            direction_forward = true;
        }
    }
}

      uint8_t ch = getchar();
      cli(ch);
      usleep(10000);
    }
  }


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
  };  // Infinite event loop
}
