#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "tone_lut.h"

static const char *TAG = "tone_scale";

#define SAMPLE_RATE 24000
#define I2S_BCLK_IO 4
#define I2S_LRCLK_IO 5
#define I2S_DOUT_IO 6

#define NOTE_DURATION_MS 300
#define CHANNELS 2

i2s_chan_handle_t tx_handle;

// Do–Re–Mi–Fa–So–La–Xi–Do (C4 ~ C5)
static const float notes[] = {
    261.63f, // Do
    293.66f, // Re
    329.63f, // Mi
    349.23f, // Fa
    392.00f, // So
    440.00f, // La
    493.88f, // Xi
    523.25f  // Do (C5)
};

void audio_i2s_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK_IO,
            .ws = I2S_LRCLK_IO,
            .dout = I2S_DOUT_IO,
            .din = I2S_GPIO_UNUSED,
        }};

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));
    ESP_LOGI(TAG, "I2S initialized");
}

void play_note(float freq, int duration_ms)
{
    int total_samples = SAMPLE_RATE * duration_ms / 1000;
    int16_t *buffer = malloc(total_samples * CHANNELS * sizeof(int16_t));
    if (!buffer)
    {
        ESP_LOGE(TAG, "Failed to alloc buffer");
        return;
    }

    for (int i = 0; i < total_samples; ++i)
    {
        float t = (float)i / SAMPLE_RATE;                  // Current sampling time
        float phase = fmodf(t * freq, 1.0f);               // Phase of the current sample point (0.0 - 1.0)
        int16_t sample = get_lut_sample_from_phase(phase); // Sampling

        buffer[2 * i] = sample;     // Left
        buffer[2 * i + 1] = sample; // Right
    }

    size_t written;
    i2s_channel_write(tx_handle, buffer, total_samples * CHANNELS * sizeof(int16_t), &written, portMAX_DELAY);
    free(buffer);
}

void tone_scale_loop_play(void)
{
    ESP_LOGI(TAG, "Playing Do–Re–Mi–Fa–So–La–Xi–Do loop...");
    while (1)
    {
        for (int i = 0; i < sizeof(notes) / sizeof(notes[0]); ++i)
        {
            play_note(notes[i], NOTE_DURATION_MS);
        }
    }
}

void app_main(void)
{
    audio_i2s_init();
    generate_lut();         // Generate a sine wave lookup table using tone_lut
    tone_scale_loop_play(); // Infinite playback of musical scales
}
