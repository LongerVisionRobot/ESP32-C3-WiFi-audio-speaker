#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#include "qrcode.h"

static const char *TAG = "lvt_ble_prov";
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_EVENT = BIT0;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_PROV_EVENT)
    {
        switch (event_id)
        {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;
        case WIFI_PROV_CRED_RECV:
        {
            wifi_sta_config_t *cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Received Wi-Fi credentials: SSID=%s, PASSWORD=%s", (char *)cfg->ssid, (char *)cfg->password);
            break;
        }
        case WIFI_PROV_CRED_FAIL:
            ESP_LOGE(TAG, "Provisioning failed! Please reset and try again.");
            break;
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning successful");
            break;
        case WIFI_PROV_END:
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected, got IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
    }
    else if (event_base == PROTOCOMM_TRANSPORT_BLE_EVENT)
    {
        if (event_id == PROTOCOMM_TRANSPORT_BLE_CONNECTED)
            ESP_LOGI(TAG, "BLE transport: Connected!");
        else if (event_id == PROTOCOMM_TRANSPORT_BLE_DISCONNECTED)
            ESP_LOGI(TAG, "BLE transport: Disconnected!");
    }
}

static void get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "PROV_%02X%02X%02X", eth_mac[3], eth_mac[4], eth_mac[5]);
}

void lvt_ble_wifi_prov_start(void)
{
    ESP_LOGI(TAG, "Starting BLE WiFi Provisioning");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_TRANSPORT_BLE_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM};
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    if (!provisioned)
    {
        ESP_LOGI(TAG, "Not provisioned, start BLE provisioning");
        char service_name[12];
        get_device_service_name(service_name, sizeof(service_name));
        const char *pop = "abcd1234";
        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, (const void *)pop, service_name, NULL));

        // Print QR code for provisioning
        char payload[150];
        snprintf(payload, sizeof(payload), "{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"ble\"}", service_name, pop);
        ESP_LOGI(TAG, "Scan QR code in ESP BLE Provisioning app:");
        esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
        esp_qrcode_generate(&cfg, payload);

        ESP_LOGI(TAG, "Or use URL:\nhttps://espressif.github.io/esp-jumpstart/qrcode.html?data=%s", payload);
    }
    else
    {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");
        wifi_prov_mgr_deinit();
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
    }

    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, true, true, portMAX_DELAY);
}
