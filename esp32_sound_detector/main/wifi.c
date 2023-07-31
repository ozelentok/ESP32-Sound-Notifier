// clang-format off
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
// clang-format on
#include <assert.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <lwip/sockets.h>

#define ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD

#define WIFI_CONNECTED_BIT BIT0
#define CONNECTION_GRACE_TIME_MS (5000)

static const char *TAG = "WiFi";
static EventGroupHandle_t s_wifi_event_group;
static int s_is_connected = 0;

static void wifi_event_handler(void *arg, const esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT) {
    if (event_id == WIFI_EVENT_STA_START) {
      ESP_LOGI(TAG, "WiFi Station starting");
      esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
      ESP_LOGI(TAG, "WiFi station connected");
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
      ESP_LOGI(TAG, "WiFi station disconnected");
      s_is_connected = 0;
      esp_wifi_connect();
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    const ip_event_got_ip_t *event = (const ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&event->ip_info.ip));
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    s_is_connected = 1;
  }
}

int init_wifi_client(void) {
  s_wifi_event_group = xEventGroupCreate();
  assert(s_wifi_event_group != NULL);

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL,
      &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL,
      &instance_got_ip));

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = ESP_WIFI_SSID,
              .password = ESP_WIFI_PASS,
          },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MAX_MODEM));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "Waiting for WiFi to connect");
  vTaskDelay(CONNECTION_GRACE_TIME_MS / portTICK_PERIOD_MS);

  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);

  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "Connected to ap SSID: %s", ESP_WIFI_SSID);
    return 0;
  }

  ESP_LOGE(TAG, "Unexpected Event");
  return 1;
}

int is_wifi_connected(void) { return s_is_connected; }
