#ifndef PLAYBACK_DEVICE_H
#define PLAYBACK_DEVICE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

typedef struct {
    int samplerate;
    int bits;
    int channels;
    char *name;
    //开发者在open接口内指定
    int frame_size;
} playback_device_cfg_t;

struct playback_device {
    int (*open)(struct playback_device *self, playback_device_cfg_t *cfg);
    int (*start)(struct playback_device *self);
    int (*write)(struct playback_device *self, const char *data, size_t data_len);
    int (*stop)(struct playback_device *self);
    int (*abort)(struct playback_device *self);
    void (*close)(struct playback_device *self);
    void *userdata;
};

typedef struct playback_device playback_device_t;

#ifdef __cplusplus
}
#endif
#endif
