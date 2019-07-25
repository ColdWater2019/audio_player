#ifndef RECORDER_H
#define RECORDER_H
#ifdef __cplusplus
extern "C" {
#endif
#include "record_writer.h"
#include "record_encoder.h"
#include "capture_device.h"

__attribute ((visibility("default"))) int recorder_init();

//注册解码器，默认支持pcm/wav/mp3/aac
//开发者可以自行实现其他解码器，通过此接口进行注册
//可以参考record_pcm.c实现
//支持覆盖
__attribute ((visibility("default"))) int recorder_register_encoder(const char *type, record_encoder_t *encoder);


typedef struct recorder* recorder_handle_t;

typedef enum {
    RECORD_INFO_SUCCESS = 0,
    //预处理出错
    RECORD_INFO_WRITER,
    //解码出错
    RECORD_INFO_ENCODE,
    //暂停
    RECORD_INFO_PAUSED,
    //继续
    RECORD_INFO_RESUMED,
    //正常结束
    RECORD_INFO_IDLE,
    //终止播放
    RECORD_INFO_STOP,
} record_info_t;

//播放器内部操作回调
typedef void (*recorder_listen_cb)(recorder_handle_t self, record_info_t info, void *userdata);

typedef struct {
    //当前播放器实例标签
    const char *tag;
    //某些物理设备需要指定名称
    char *device_name;
    //具体的物理设备实现
    capture_device_t device;
    //预处理缓冲区大小
    size_t record_buf_size;
    //解码缓冲区大小
    size_t encode_buf_size;

    //可选
    recorder_listen_cb listen;
    void *userdata;
}recorder_cfg_t;

__attribute ((visibility("default"))) recorder_handle_t recorder_create(recorder_cfg_t *cfg);

typedef struct {
    //文件路径/网络地址/内存地址等等具体的含义
    char *target;
    char *type;
    bool need_free;
    //指定当前要播放的音频的预处理器
    //当前播放器自带DEFAULT_FILE_writer/DEFAULT_HTTP_writer
    //开发者可以自行实现record_writer_t结构
    record_writer_t writer;

    //当pcm文件时需要手动指定
    int samplerate;
    int bits;
    int channels;
} record_cfg_t;

__attribute ((visibility("default"))) int recorder_record(recorder_handle_t self, record_cfg_t *cfg);

typedef enum {
    //空闲状态
    recorder_STATE_IDLE = 0,
    //运行状态
    recorder_STATE_RUNNING,
    //暂停状态
    recorder_STATE_PAUSED
}recorder_state_t;

__attribute ((visibility("default"))) recorder_state_t recorder_get_state(recorder_handle_t self);
__attribute ((visibility("default"))) int recorder_stop(recorder_handle_t self);
__attribute ((visibility("default"))) int recorder_pause(recorder_handle_t self);
__attribute ((visibility("default"))) int recorder_resume(recorder_handle_t self);
__attribute ((visibility("default"))) int recorder_wait_idle(recorder_handle_t self);
__attribute ((visibility("default"))) void recorder_destroy(recorder_handle_t self);
__attribute ((visibility("default"))) void recorder_deinit();

#ifdef __cplusplus
}
#endif
#endif
