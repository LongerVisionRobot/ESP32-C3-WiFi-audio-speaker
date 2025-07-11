#include "audio_i2s.h"
#include "driver/i2s_std.h"
#include "esp_log.h"

static const char *TAG = "audio_i2s";

i2s_chan_handle_t tx_handle;

#define SAMPLE_RATE 24000
#define I2S_BCLK_IO 4  // MAX98357A BCLK → GPIO4
#define I2S_LRCLK_IO 5 // MAX98357A LRC  → GPIO5
#define I2S_DOUT_IO 6  // MAX98357A DIN  → GPIO6

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
    ESP_LOGI(TAG, "I2S initialized at %d Hz", SAMPLE_RATE);
}
