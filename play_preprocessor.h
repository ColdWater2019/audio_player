#ifndef PLAY_PREPROCESSOR_H
#define PLAY_PREPROCESSOR_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>

//开发者根据接口声明实现特定的预处理操作

typedef struct {
    char *target;
    //当前播放器实例标签
    const char *tag;
    //开发者在init接口中指定
    int frame_size;
    //开发者在init接口中指定解码器类型，例如"pcm","wav","mp3"
    char *type;
} play_preprocessor_cfg_t;

struct play_preprocessor {
    const char *type;
    //接口调用成功，需要指定cfg->type的值来告知解码类型
    int (*init)(struct play_preprocessor *self, play_preprocessor_cfg_t *cfg);
    //sdk中会主动调用此接口来读取数据，data_len的长度为frame_size;
    int (*read)(struct play_preprocessor *self, char *data, size_t data_len);
    void (*destroy)(struct play_preprocessor *self);
    //sdk中不操作此字段，开发者可以将自己的数据结构挂载在此指针上
    void *userdata;
};

typedef struct play_preprocessor play_preprocessor_t;


#ifdef __cplusplus
}
#endif
#endif
