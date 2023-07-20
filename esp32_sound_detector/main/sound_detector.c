// clang-format off
#include <freertos/FreeRTOS.h>
#include "freertos/queue.h"
// clang-format on
#include <assert.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_pm.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <lwip/sockets.h>

#include "wifi.h"

#define REMOTE_IP CONFIG_SOUND_NOTIFICATION_REMOTE_IP
#define REMOTE_PORT CONFIG_SOUND_NOTIFICATION_REMOTE_PORT
#define AUTH_SECRET CONFIG_SOUND_NOTIFICATION_SECRET

#define SOUND_DIGITAL_PIN GPIO_NUM_34
#define POST_NOTIFICATION_DOWNTIME_DURATION_MS (2 * 1000)

#define ESP_INTR_FLAG_DEFAULT (0)

static const char *TAG = "SoundDetector";
static QueueHandle_t gpio_evt_queue = NULL;

static void send_notification(void) {
  int sock = -1;

  struct sockaddr_in dest_addr;
  dest_addr.sin_addr.s_addr = inet_addr(REMOTE_IP);
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(REMOTE_PORT);

  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock == -1) {
    ESP_LOGE(TAG, "Failed creating socket, errno %d", errno);
    goto cleanup;
  }

  int err = sendto(sock, AUTH_SECRET, strlen(AUTH_SECRET), 0,
                   (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  if (err < 0) {
    ESP_LOGE(TAG, "Failed to send, errno %d", errno);
    goto cleanup;
  }
  ESP_LOGI(TAG, "Sent to %s:%d", REMOTE_IP, REMOTE_PORT);

cleanup:
  if (sock != -1) {
    shutdown(sock, 0);
    close(sock);
    sock = -1;
  }
}

static void gpio_isr_handler(void *arg) {
  const uint16_t gpio_num = GPIO_NUM_34;
  gpio_intr_disable(gpio_num);
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void sound_interrupt_handler(void *arg) {
  uint16_t io_num = 0;
  while (1) {
    if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
      send_notification();
      vTaskDelay(POST_NOTIFICATION_DOWNTIME_DURATION_MS / portTICK_PERIOD_MS);
      gpio_intr_enable(io_num);
    }
  }
}

static void init_pm(void) {
  esp_pm_config_t pm_config = {
      .max_freq_mhz = 160, .min_freq_mhz = 40, .light_sleep_enable = 1};
  ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
}

static void init_gpio(void) {
  gpio_set_direction(SOUND_DIGITAL_PIN, GPIO_MODE_INPUT);
  gpio_set_intr_type(SOUND_DIGITAL_PIN, GPIO_INTR_HIGH_LEVEL);

  gpio_evt_queue = xQueueCreate(10, sizeof(uint16_t));
  assert(gpio_evt_queue != NULL);

  xTaskCreate(sound_interrupt_handler, "sound_interrupt_handler", 2048, NULL,
              10, NULL);
  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  gpio_isr_handler_add(SOUND_DIGITAL_PIN, gpio_isr_handler, NULL);
}

void app_main(void) {
  init_pm();
  init_wifi_client();
  init_gpio();
}
