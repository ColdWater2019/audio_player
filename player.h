#ifndef PLAYER_H
#define PLAYER_H
#ifdef __cplusplus
extern "C" {
#endif
#include "play_preprocessor.h"
#include "play_decoder.h"
#include "playback_device.h"

__attribute ((visibility("default"))) int player_init();

//注册解码器，默认支持pcm/wav/mp3/aac
//开发者可以自行实现其他解码器，通过此接口进行注册
//可以参考play_pcm.c实现
//支持覆盖
__attribute ((visibility("default"))) int player_register_decoder(const char *type, play_decoder_t *decoder);


typedef struct player* player_handle_t;

typedef enum {
    PLAY_INFO_SUCCESS = 0,
    //预处理出错
    PLAY_INFO_PREPROCESS,
    //解码出错
    PLAY_INFO_DECODE,
    //暂停
    PLAY_INFO_PAUSED,
    //继续
    PLAY_INFO_RESUMED,
    //正常结束
    PLAY_INFO_IDLE,
    //终止播放
    PLAY_INFO_STOP,
} play_info_t;

//播放器内部操作回调
typedef void (*player_listen_cb)(player_handle_t self, play_info_t info, void *userdata);

typedef struct {
    //当前播放器实例标签
    const char *tag;
    //某些物理设备需要指定名称
    char *name;
    //具体的物理设备实现
    playback_device_t device;
    //预处理缓冲区大小
    size_t preprocess_buf_size;
    //解码缓冲区大小
    size_t decode_buf_size;

    //可选
    player_listen_cb listen;
    void *userdata;
}player_cfg_t;

__attribute ((visibility("default"))) player_handle_t player_create(player_cfg_t *cfg);

typedef struct {
    //文件路径/网络地址/内存地址等等具体的含义
    char *target;
    bool need_free;
    //指定当前要播放的音频的预处理器
    //当前播放器自带DEFAULT_FILE_PREPROCESSOR/DEFAULT_HTTP_PREPROCESSOR
    //开发者可以自行实现play_preprocessor_t结构
    play_preprocessor_t preprocessor;

    //当pcm文件时需要手动指定
    int samplerate;
    int bits;
    int channels;
} play_cfg_t;

__attribute ((visibility("default"))) int player_play(player_handle_t self, play_cfg_t *cfg);

typedef enum {
    //空闲状态
    PLAYER_STATE_IDLE = 0,
    //运行状态
    PLAYER_STATE_RUNNING,
    //暂停状态
    PLAYER_STATE_PAUSED
}player_state_t;

__attribute ((visibility("default"))) player_state_t player_get_state(player_handle_t self);
__attribute ((visibility("default"))) int player_stop(player_handle_t self);
__attribute ((visibility("default"))) int player_pause(player_handle_t self);
__attribute ((visibility("default"))) int player_resume(player_handle_t self);
__attribute ((visibility("default"))) int player_wait_idle(player_handle_t self);
__attribute ((visibility("default"))) void player_destroy(player_handle_t self);
__attribute ((visibility("default"))) void player_deinit();

#ifdef __cplusplus
}
#endif
#endif
