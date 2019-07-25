#include "file_preprocessor.h"
#include <string.h>
#include <stdio.h>
#include "os_log.h"
os_log_create_module(file_preprocessor,OS_LOG_LEVEL_INFO);


static FILE *check_native_audio_type(char *target, char **type) {
    char *p = strrchr(target, '.');
    if (!p) return NULL;
    if (0 == strcasecmp(p, ".pcm")) {
        *type = "pcm";
    } else if (0 == strcasecmp(p, ".wav")) {
        *type = "wav";
    } else if (0 == strcasecmp(p, ".mp3")) {
        *type = "mp3";
    } else if (0 == strcasecmp(p, ".aac")) {
        *type = "aac";
    } else if (0 == strcasecmp(p, ".flac")) {*type = "flac";
    } else {
        return NULL;
    }
    return fopen(target, "rb");
}
int file_preprocessor_init_impl(struct play_preprocessor *self,
        play_preprocessor_cfg_t *cfg) {
    OS_LOG_D(file_preprocessor,"file_preprocessor_init_impl in \n");
    FILE *fd = check_native_audio_type(cfg->target, &cfg->type);
    if (!fd) {
        OS_LOG_D(file_preprocessor,"[%s]open native file error, file: %s",cfg->tag, cfg->target);
        return -1;
    }
    
    OS_LOG_D(file_preprocessor,"file_preprocessor_init_impl in 1\n");
    cfg->frame_size = 512;
    self->userdata = (void *)fd;
    OS_LOG_D(file_preprocessor,"[%s]open native file ok, file: %s, audio type:%s\n", cfg->tag, cfg->target, cfg->type);
    return 0;
}
int file_preprocessor_read_impl(struct play_preprocessor *self, char *data,
        size_t data_len) {
    FILE *fd = (FILE *)self->userdata;
    OS_LOG_D(file_preprocessor,"file_preprocessor_read_impl read data_len:%d\n",data_len);
    return fread(data, 1, data_len, fd);
}
void file_preprocessor_destroy_impl(struct play_preprocessor *self) {
    if (!self) return;
    FILE *fd = (FILE *)self->userdata;
    fclose(fd);
}
