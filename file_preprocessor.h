#ifndef FILE_PREPROCESSOR_H
#define FILE_PREPROCESSOR_H
#ifdef __cplusplus
extern "C" {
#endif

#include "play_preprocessor.h"

//当前播放本地文件的实现中只支持文件名后缀为.pcm/.wav/.mp3/.aac的音频

__attribute ((visibility("default"))) int file_preprocessor_init_impl(struct play_preprocessor *self, play_preprocessor_cfg_t *cfg);
__attribute ((visibility("default"))) int file_preprocessor_read_impl(struct play_preprocessor *self, char *data, size_t data_len);
__attribute ((visibility("default"))) void file_preprocessor_destroy_impl(struct play_preprocessor *self);

#define DEFAULT_FILE_PREPROCESSOR { \
        .type = "file", \
        .init = file_preprocessor_init_impl, \
        .read = file_preprocessor_read_impl, \
        .destroy = file_preprocessor_destroy_impl \
    }

#ifdef  __cplusplus
}
#endif
#endif
