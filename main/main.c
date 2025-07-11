#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "wifi_prov.h"
#include "audio_i2s.h"
#include "opus_stream.h"

static const char *TAG = "main";

// 🔊 Audio playback task
void audio_task(void *param)
{
    const char *input_url = (const char *)param;

    // Copy URL locally (on task stack, does not rely on external malloc)
    char url[256];
    strncpy(url, input_url, sizeof(url) - 1);
    url[sizeof(url) - 1] = '\0';

    // audio_task
    ESP_LOGI("audio_task", "Got URL: %s", url);
    ESP_LOGI("audio_task", "Free stack before play: %lu", uxTaskGetStackHighWaterMark(NULL));

    audio_i2s_init();
    // ESP_LOGI("audio_task", "Free stack: %lu", uxTaskGetStackHighWaterMark(NULL));

    opus_stream_play(url);

    free(param); // Release memory allocated by strdup last
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting BLE WiFi provisioning...");
    lvt_ble_wifi_prov_start(); // Start provisioning (skip if already done)

    ESP_LOGI(TAG, "Waiting for WiFi to connect...");
    vTaskDelay(pdMS_TO_TICKS(10000)); // Wait for IP acquisition; can be replaced with event group sync

    // Play WebRadio (by creating a new task)
    // ✅ Correct way: dynamically allocate string
    char *url = strdup("http://192.168.1.100:8000/stream.opus");

    // Do not play directly here (if you want to use a task to play)
    xTaskCreate(audio_task, "audio", 16384, (void *)url, 5, NULL);
}
