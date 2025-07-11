idf_component_register(SRCS "main.c" "wifi_prov.c" "audio_i2s.c" "opus_stream.c"
                      INCLUDE_DIRS "."
                      REQUIRES nvs_flash esp_netif esp_event esp_wifi
                               wifi_provisioning driver esp_http_client
                               bt)
