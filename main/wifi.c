// Code taken from the ESP32 IDF WiFi station Example

#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_softap.h>
#include "lwip/err.h"
#include "lwip/sys.h"

#include "mongoose.h"
#include "main.h"

//char *config_read(void);
//void config_write(struct mg_str config);

typedef const char wifi_prov_security1_params_t;

static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about
 * two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
#ifdef CONFIG_EXAMPLE_RESET_PROV_MGR_ON_FAILURE
  static int retries;
#endif

  if (event_base == WIFI_PROV_EVENT)
  {
    switch (event_id)
    {
    case WIFI_PROV_START:
      MG_INFO(("Provisioning started"));
      break;
    case WIFI_PROV_CRED_RECV:
    {
      wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
      MG_INFO(("Received Wi-Fi credentials"
               "\n\tSSID     : %s\n\tPassword : %s",
               (const char *)wifi_sta_cfg->ssid,
               (const char *)wifi_sta_cfg->password));


      MG_INFO(("Store Wi-Fi credentials"));

      mg_file_printf(&mg_fs_posix, WIFI_FILE, "{%m:%m,%m:%m}\n", mg_print_esc, 0,
                   "ssid", mg_print_esc, 0, (const char *)wifi_sta_cfg->ssid, mg_print_esc, 0, "pass",
                   mg_print_esc, 0, (const char *)wifi_sta_cfg->password);




      break;
    }
    case WIFI_PROV_CRED_FAIL:
    {
      wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
      MG_ERROR(("Provisioning failed!\n\tReason : %s"
                "\n\tPlease reset to factory and retry provisioning",
                (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Wi-Fi station authentication failed" : "Wi-Fi access-point not found"));
#ifdef CONFIG_EXAMPLE_RESET_PROV_MGR_ON_FAILURE
      retries++;
      if (retries >= CONFIG_EXAMPLE_PROV_MGR_MAX_RETRY_CNT)
      {
        MG_INFO("Failed to connect with provisioned AP, reseting provisioned credentials");
        wifi_prov_mgr_reset_sm_state_on_failure();
        retries = 0;
      }
#endif
      break;
    }
    case WIFI_PROV_CRED_SUCCESS:
    {
      wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;

      MG_INFO(("Provisioning successful"));





#ifdef CONFIG_EXAMPLE_RESET_PROV_MGR_ON_FAILURE
      retries = 0;
#endif
      break;
    }
    case WIFI_PROV_END:
      /* De-initialize manager once provisioning is finished */
      wifi_prov_mgr_deinit();

//      char *config = config_read();

//      config_write(mg_str(config));

      //    if (config != NULL) config_apply(mg_str(config));
      //free(config);

      break;
    default:
      break;
    }
  }
  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
  {
    esp_wifi_connect();
  }
  else if (event_base == WIFI_EVENT &&
           event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    gpio_set_level(GPIO_NUM_22, 0);
    esp_wifi_disconnect();
    esp_wifi_connect();

    //    if (s_retry_num < 3) {
    //      s_retry_num++;
    //      MG_INFO(("retry to connect to the AP"));
    //    } else {
    //      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    //    }
    //    MG_ERROR(("connect to the AP fail"));
  }
  else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    MG_INFO(("IP ADDRESS: " IPSTR ". Go to:", IP2STR(&event->ip_info.ip)));
    MG_INFO(("http://" IPSTR, IP2STR(&event->ip_info.ip)));
    gpio_set_level(GPIO_NUM_22, 1);
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

esp_err_t custom_prov_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                   uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
  if (inbuf)
  {
    MG_INFO(("Received data: %.*s", inlen, (char *)inbuf));
  }
  char response[] = "SUCCESS";
  *outbuf = (uint8_t *)strdup(response);
  if (*outbuf == NULL)
  {
    MG_ERROR(("System out of memory"));
    return ESP_ERR_NO_MEM;
  }
  *outlen = strlen(response) + 1; /* +1 for NULL terminating byte */

  return ESP_OK;
}

static void get_device_service_name(char *service_name, size_t max)
{
  uint8_t eth_mac[6];
  const char *ssid_prefix = "PROV_";
  esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
  snprintf(service_name, max, "%s%02X%02X%02X",
           ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

bool wifi_prev(){
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_netif_init());

  /* Initialize the event loop */
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  s_wifi_event_group = xEventGroupCreate();


  return true;
}

bool wifi_init_prov()
{
  /* Initialize TCP/IP */


  char service_name[12];
  get_device_service_name(service_name, sizeof(service_name));
  const char *service_key = NULL;
  const char *pop = "abcd1234";
  wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
  /* This is the structure for passing security parameters
   * for the protocomm security 1.
   */
  wifi_prov_security1_params_t *sec_params = pop;


  /* Register our event handler for Wi-Fi, IP and Provisioning related events */
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

  /* Initialize Wi-Fi including netif with default config */
  esp_netif_create_default_wifi_sta();
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  /* Configuration for the provisioning manager */
  wifi_prov_mgr_config_t config = {
      .scheme = wifi_prov_scheme_softap,
      .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE};

  /* Initialize provisioning manager with the
   * configuration parameters set above */
  ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

  bool provisioned = false;
  wifi_prov_mgr_reset_provisioning();

  wifi_prov_mgr_endpoint_create("custom-data");
  ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, (const void *)sec_params, service_name, service_key));

  wifi_prov_mgr_endpoint_register("custom-data", custom_prov_data_handler, NULL);

  ESP_ERROR_CHECK(esp_wifi_start());

  /* Let's find out if the device is provisioned */
  //    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

  return true;
}

bool wifi_init(const char *ssid, const char *pass)
{
  bool result = false;
  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;

  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

  wifi_config_t c = {.sta = {.threshold = {.authmode = WIFI_AUTH_WPA2_PSK},
                             .pmf_cfg = {.capable = true, .required = false}}};
  snprintf((char *)c.sta.ssid, sizeof(c.sta.ssid), "%s", ssid);
  snprintf((char *)c.sta.password, sizeof(c.sta.password), "%s", pass);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &c));
  ESP_ERROR_CHECK(esp_wifi_start());

  MG_DEBUG(("wifi_init_sta finished."));

  //  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
  //                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
  //                                         pdFALSE, pdFALSE, portMAX_DELAY);
  /*
    if (bits & WIFI_CONNECTED_BIT) {
      MG_INFO(("connected to ap SSID:%s", ssid));
      result = true;
    } else if (bits & WIFI_FAIL_BIT) {
      MG_ERROR(("Failed to connect to SSID:%s, password:%s", ssid, pass));
    } else {
      MG_ERROR(("UNEXPECTED EVENT"));
    }
  */
  /* The event will not be processed after unregister */
  //  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
  //      IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
  //  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
  //      WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
  //  vEventGroupDelete(s_wifi_event_group);
  return true;
}
