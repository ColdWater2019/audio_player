#ifndef HTTP_PREPROCESSOR_H
#define HTTP_PREPROCESSOR_H
#ifdef __cplusplus
extern "C" {
#endif

#include "play_preprocessor.h"

//当前只实现了http音频流，用于播放wav/mp3/aac音频
__attribute ((visibility("default"))) int http_preprocessor_init_impl(struct play_preprocessor *self, play_preprocessor_cfg_t *cfg);
__attribute ((visibility("default"))) int http_preprocessor_read_impl(struct play_preprocessor *self, char *data, size_t data_len);
__attribute ((visibility("default"))) void http_preprocessor_destroy_impl(struct play_preprocessor *self);

#define DEFAULT_HTTP_PREPROCESSOR { \
        .type = "http", \
        .init = http_preprocessor_init_impl, \
        .read = http_preprocessor_read_impl, \
        .destroy = http_preprocessor_destroy_impl \
    }

#ifdef __cplusplus
}
#endif
#endif
