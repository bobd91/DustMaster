#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_now.h"

static const char *TAG = "DustMaster";

static const uint8_t welcome[] = { 'W', 'E', 'L', 'C', 'O', 'M', 'E' };

static void nvs_init(void)
{
      // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
}

static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start());
}

static char *data_as_string(const uint8_t *data, int len)
{
  char *r = malloc(len + 1);
  if(r) {
    memcpy(r, data, len);
    r[len] = '\0';
  }
  return r;  
}

static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  //if(ESP_NOW_SEND_SUCCESS != status) 
  //{
    ESP_LOGD(TAG, "espnow_send_cb: mac=" MACSTR ", status=%0x", MAC2STR(mac_addr), status);
  //}
}

static void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
  ESP_LOGD(TAG, "espnow_recv_cb");

  char *dstring = data_as_string(data, len);
  ESP_LOGD(TAG, "espnow_recv_cb: mac=" MACSTR ", len=%d, data=%s", 
      MAC2STR(mac_addr), len, dstring);
  
  if(strcmp("HELLO", dstring) == 0)
  {
    if(!esp_now_is_peer_exist(mac_addr)) {
      esp_now_peer_info_t peer_info = {
        .channel = 0,
        .encrypt = false
      };
      memcpy(peer_info.peer_addr, mac_addr, sizeof(peer_info.peer_addr));
      esp_now_add_peer(&peer_info);
    }
    esp_err_t res = esp_now_send(mac_addr, welcome, sizeof(welcome));
    if(ESP_OK != res) 
    {
      ESP_LOGE(TAG, "espnow_recv_cb: send welcome failed: res=%x", res);
    }
  }
  free(dstring);
}

static esp_err_t espnow_init(void)
{
    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(espnow_recv_cb) );

    return ESP_OK;
}

void app_main(void)
{
  nvs_init();
  wifi_init();
  espnow_init();

  esp_log_level_set(TAG, ESP_LOG_DEBUG);

  while(1)
    vTaskDelay(1);
}