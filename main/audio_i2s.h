#pragma once

#include "driver/i2s_std.h"

#ifdef __cplusplus
extern "C"
{
#endif

    extern i2s_chan_handle_t tx_handle;

    void audio_i2s_init(void);

#ifdef __cplusplus
}
#endif
