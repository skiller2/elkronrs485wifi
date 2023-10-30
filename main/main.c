// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved

#include "main.h"

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

  if (json != NULL && gpio_get_level(GPIO_NUM_18)==0) {
    char *ssid = mg_json_get_str(mg_str(json), "$.ssid");
    char *pass = mg_json_get_str(mg_str(json), "$.pass");
    wifi_init(ssid, pass);
    free(ssid);
    free(pass);
    free(json);
  } else {
    // If WiFi is not configured, run CLI until configured
    MG_INFO(("WiFi is not configured, running CLI. Press enter"));
    cli_init();
    for (;;) {
      uint8_t ch = getchar();
      cli(ch);
      usleep(10000);
    }
  }


  struct mg_mgr mgr;
  mg_mgr_init(&mgr);
  mg_log_set(MG_LL_DEBUG);  // Set log level
  MG_INFO(("Mongoose v%s on %s", MG_VERSION, s_listening_url));
  mg_http_listen(&mgr, s_listening_url, uart_bridge_fn, &mgr);
//  uint64_t last_ms = 0;

  for (;;) {
    gpio_set_level(GPIO_NUM_2, 0);
    mg_mgr_poll(&mgr, 150);
    gpio_set_level(GPIO_NUM_2, 1);
    mg_mgr_poll(&mgr, 150);
  };  // Infinite event loop
}
