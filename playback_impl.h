#ifndef PLAYER_IMPL_H
#define PLAYER_IMPL_H
#ifdef __cplusplus
extern "C" {
#endif

#include "playback_device.h"

int playback_device_open_impl(struct playback_device *self, playback_device_cfg_t *cfg);
int playback_device_start_impl(struct playback_device *self);
int playback_device_write_impl(struct playback_device *self, const char *data, size_t data_len);
int playback_device_stop_impl(struct playback_device *self);
int playback_device_abort_impl(struct playback_device *self);
void playback_device_close_impl(struct playback_device *self);

#define DEFAULT_PLAYBACK_DEVICE { \
        .open = playback_device_open_impl, \
        .start = playback_device_start_impl, \
        .write = playback_device_write_impl, \
        .stop = playback_device_stop_impl, \
        .abort = playback_device_abort_impl, \
        .close = playback_device_close_impl, \
    }

#ifdef __cplusplus
}
#endif
#endif
