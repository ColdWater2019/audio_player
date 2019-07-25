#ifndef FILE_WRITER_H
#define FILE_WRITER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "record_writer.h"

//当前播放本地文件的实现中只支持文件名后缀为.pcm/.wav/.mp3/.aac的音频

__attribute ((visibility("default"))) int file_writer_init_impl(struct record_writer *self, record_writer_cfg_t *cfg);
__attribute ((visibility("default"))) int file_writer_write_impl(struct record_writer *self, char *data, size_t data_len);
__attribute ((visibility("default"))) void file_writer_destroy_impl(struct record_writer *self);

#define DEFAULT_FILE_WRITER { \
        .type = "file", \
        .init = file_writer_init_impl, \
        .write = file_writer_write_impl, \
        .destroy = file_writer_destroy_impl \
    }

#ifdef  __cplusplus
}
#endif
#endif
