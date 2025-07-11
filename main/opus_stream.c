#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "driver/i2s_std.h"
#include "opus.h"
#include "audio_i2s.h"

extern i2s_chan_handle_t tx_handle;

static const char *TAG = "opus_stream";

#define SAMPLE_RATE 24000
#define CHANNELS 2
#define PCM_FRAME_SIZE 480 // Maximum possible PCM frame samples
#define PCM_BUF_SIZE (PCM_FRAME_SIZE * CHANNELS)
#define READ_BUF_SIZE 1024

void opus_stream_play(const char *url)
{
    ESP_LOGI(TAG, "Streaming from URL: %s", url);

    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t http = esp_http_client_init(&config);
    if (!http || esp_http_client_open(http, 0) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open HTTP stream");
        return;
    }

    // ✅ Insert position: After the connection is successful,
    // fetch the response header information.
    esp_http_client_fetch_headers(http);
    int content_length = esp_http_client_get_content_length(http);
    ESP_LOGI(TAG, "HTTP content length: %d", content_length);

    int err;
    OpusDecoder *decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &err);
    if (!decoder || err != OPUS_OK)
    {
        ESP_LOGE(TAG, "Opus decoder init failed: %d", err);
        esp_http_client_cleanup(http);
        return;
    }

    uint8_t *read_buf = malloc(READ_BUF_SIZE);
    int16_t *pcm_buf = malloc(PCM_BUF_SIZE * sizeof(int16_t));

    if (!read_buf || !pcm_buf)
    {
        ESP_LOGE(TAG, "Malloc failed");
        goto cleanup;
    }

    while (1)
    {
        // Step 1: Read 2-byte to get the frame length
        uint8_t len_buf[2];
        int len_read = 0;
        while (len_read < 2)
        {
            int r = esp_http_client_read(http, (char *)(len_buf + len_read), 2 - len_read);
            if (r <= 0)
            {
                ESP_LOGI(TAG, "Stream ended or error while reading length.");
                goto done;
            }
            len_read += r;
        }
        int frame_len = (len_buf[0] << 8) | len_buf[1];
        if (frame_len <= 0 || frame_len > READ_BUF_SIZE)
        {
            ESP_LOGD(TAG, "Invalid frame_len: %d", frame_len);
            continue;
        }

        // Step 2: Read that many bytes as frame data
        int total_read = 0;
        while (total_read < frame_len)
        {
            int r = esp_http_client_read(http, (char *)(read_buf + total_read), frame_len - total_read);
            if (r <= 0)
            {
                ESP_LOGW(TAG, "Failed to read full frame. Got %d of %d", total_read, frame_len);
                break;
            }
            total_read += r;
        }
        if (total_read < frame_len)
            continue;

        // Step 3: Decode Opus frame
        int decoded_samples = opus_decode(decoder, read_buf, frame_len, pcm_buf, PCM_FRAME_SIZE, 0);
        if (decoded_samples < 0)
        {
            ESP_LOGW(TAG, "Decode error: %d", decoded_samples);
            continue;
        }

        // Step 4: Output PCM to I2S
        size_t written = 0;
        i2s_channel_write(tx_handle, pcm_buf, decoded_samples * CHANNELS * sizeof(int16_t), &written, portMAX_DELAY);
    }

done:
    ESP_LOGI(TAG, "Opus stream finished");

cleanup:
    if (decoder)
        opus_decoder_destroy(decoder);
    if (http)
        esp_http_client_cleanup(http);
    if (read_buf)
        free(read_buf);
    if (pcm_buf)
        free(pcm_buf);
}

