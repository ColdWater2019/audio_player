#include "file_writer.h"
#include <string.h>
#include <stdio.h>
#include "os_log.h"
os_log_create_module(file_writer,OS_LOG_LEVEL_DEBUG);


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
    return fopen(target, "wb+");
}
int file_writer_init_impl(struct record_writer *self,
        record_writer_cfg_t *cfg) {
    FILE *fd = check_native_audio_type(cfg->target, &cfg->type);
    if (!fd) {
        OS_LOG_D(file_writer,"[%s]open native file error, file: %s",cfg->tag, cfg->target);
        return -1;
    }
    
    OS_LOG_D(file_writer,"file_writer_init_impl in \n");
    cfg->frame_size = 1024;
    self->userdata = (void *)fd;
    OS_LOG_D(file_writer,"[%s]open native file ok, file: %s, audio type:%s\n", cfg->tag, cfg->target, cfg->type);
    return 0;
}
int file_writer_write_impl(struct record_writer *self, char *data,
        size_t data_len) {
    FILE *fd = (FILE *)self->userdata;
    OS_LOG_D(file_writer,"file_writer_read_impl read data_len:%d\n",data_len);
    return fwrite(data, 1, data_len, fd);
}
void file_writer_destroy_impl(struct record_writer *self) {
    if (!self) return;
    FILE *fd = (FILE *)self->userdata;
    fclose(fd);
}
