#ifndef CAPTURE_DEVICE_H
#define CAPTURE_DEVICE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

typedef struct {
    int samplerate;
    int bits;
    int channels;
    char *device_name;
    //开发者在open接口内指定
    int frame_size;
} capture_device_cfg_t;

struct capture_device {
    int (*open)(struct capture_device *self, capture_device_cfg_t *cfg);
    int (*start)(struct capture_device *self);
    int (*read)(struct capture_device *self, const char *data, size_t data_len);
    int (*stop)(struct capture_device *self);
    int (*abort)(struct capture_device *self);
    void (*close)(struct capture_device *self);
    void *userdata;
};

typedef struct capture_device capture_device_t;

#ifdef __cplusplus
}
#endif
#endif
