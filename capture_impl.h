#ifndef PLAYER_IMPL_H
#define PLAYER_IMPL_H
#ifdef __cplusplus
extern "C" {
#endif

#include "capture_device.h"

int capture_device_open_impl(struct capture_device *self, capture_device_cfg_t *cfg);
int capture_device_start_impl(struct capture_device *self);
int capture_device_read_impl(struct capture_device *self, char *data, size_t data_len);
int capture_device_stop_impl(struct capture_device *self);
int capture_device_abort_impl(struct capture_device *self);
void capture_device_close_impl(struct capture_device *self);

#define DEFAULT_CAPTURE_DEVICE { \
        .open = capture_device_open_impl, \
        .start = capture_device_start_impl, \
        .read = capture_device_read_impl, \
        .stop = capture_device_stop_impl, \
        .abort = capture_device_abort_impl, \
        .close = capture_device_close_impl, \
    }

#ifdef __cplusplus
}
#endif
#endif
